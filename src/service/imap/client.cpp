#include <service/imap/client.h>
#include <cstring>
#include <esp32-hal-log.h>
#include <mbedtls/error.h>
#include <SPIFFS.h>
#include <service/imap/rfc2047.h>

// This is all a bunch of shitcode and probably will crash on any possible occasion
// However it seems to work well enough for simple email notifications 

static char LOG_TAG[] = "IMAP_NFY";

#define TXN_TAG "A001 "
#define PEM_FILE_NAME "/gmail_root_cert.pem"
static uint8_t * pem_content;
static size_t pem_size;

extern "C" void imap_task(void*);

void imap_task(void * pvParameter) {
    ImapNotifier * that = static_cast<ImapNotifier*>(pvParameter);
    if(that != nullptr) {
        while(1) {
            that->poll();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

ImapNotifier::ImapNotifier(const char * serv, int p, const char * user, const char * pass) {
    strncpy(username, user, 64);
    strncpy(password, pass, 64);
    strncpy(server, serv, 64);
    itoa(p, port, 10);
    state = DISCONNECT;
    hTask = NULL;
    did_just_expunge = false;

    connect();

    if(xTaskCreate(
        imap_task,
        "IMAP",
        8192,
        this,
        10,
        &hTask
    ) != pdPASS) {
        ESP_LOGE(LOG_TAG, "Task creation failed!");
    }
}

void ImapNotifier::set_callback(imap_notify_callback_t cb) {
    callback = cb;
}

void ImapNotifier::poll() {
    switch(state) {
        case DISCONNECT:
            connect();
            break;
        case CONNECT:
            login();
            break;

        case AUTHING:
        case SELECTING:
        case CHECKING:
        case IDLE:
            wait_lines();
            break;

        default: break;
    }
}

void ImapNotifier::wait_lines() {
    if(rxbuf_ptr >= RXBUF_SIZE) {
        ESP_LOGE(LOG_TAG, "Buffer overflow! The line did not fit in the Rx buffer. Discarding.");
        rxbuf[RXBUF_SIZE-1] = 0;
        ESP_LOGE(LOG_TAG, "For your record, it looked like: %s", rxbuf);
        rxbuf_ptr = 0;
    }
    int read = mbedtls_ssl_read(&ssl, rxbuf + rxbuf_ptr, RXBUF_SIZE - rxbuf_ptr);
    
    if(read < 0) {
        ESP_LOGE(LOG_TAG, "mbedtls_ssl_read returned -0x%x, will reconnect", -read);
        tlserror = read;
        free_socket();
        state = DISCONNECT;
        return;
    }

    rxbuf_ptr += read;
    if(read > 0) {
        char * blk = (char*) rxbuf;
        char * eol = strstr(blk, "\r\n");
        while(eol != nullptr) {
            *eol = 0;
            process_line(blk);

            blk = eol + 2; // skip over \r\n
            eol = strstr(blk, "\r\n");
        }

        if(blk < (char*) (rxbuf + rxbuf_ptr)) {
            size_t remain = (char*)(rxbuf + rxbuf_ptr) - blk;
            memcpy(rxbuf, (void*) blk, remain);
            rxbuf_ptr = remain;
            rxbuf[rxbuf_ptr] = 0;
        } else {
            rxbuf_ptr = 0;
        }
    }
}

void ImapNotifier::manual_check() {
    if(state <= CHECKING) {
        ESP_LOGE(LOG_TAG, "Not ready");
        return;
    }

    for(auto id = known_ids.begin(); id != known_ids.end(); ++id)  {
        if(callback != nullptr) {
            callback(*id, nullptr);
        }
    }
    known_ids.clear();

    send("SEARCH UNSEEN");
    state = CHECKING;
}

void ImapNotifier::download_one(imap_message_id_t id) {
    size_t ptrsz = 0;
	unsigned char *ptr = (unsigned char*) malloc(ptrsz+1);
	ptr[ptrsz] = 0;
	unsigned char buf[2048];
	size_t len;

	// Send FETCH command.
	char command[64];
	sprintf(command, "FETCH %d RFC822.HEADER", id);
    send(command);

	// Read and set null terminated string.
	while(1) {
		len = mbedtls_ssl_read(&ssl, buf, 2048);
		buf[len] = 0;
		ptrsz = ptrsz + len;
		unsigned char *tmp = (unsigned char*)realloc(ptr, ptrsz+1);
		if (tmp == NULL) {
			ESP_LOGE(LOG_TAG, "realloc fail");
			free(ptr);
			return;
		} else {
			ptr = tmp;
			strcat((char *)ptr, (char *)buf);
			ptr[ptrsz] = 0;
		}
		if (strstr((char *)buf, "A001 BAD") != 0) {
            ESP_LOGE(LOG_TAG, "Download fail");
            return;
        }
		if (strstr((char *)buf, "A001 OK") != 0) break;
	}

	char *spos;
	char *epos;
	int eslen;

    //ESP_LOGV(LOG_TAG, "buffer: %s", ptr);

    imap_message_info_t msg = { 0 };
    char name[64] = { 0 };
    char mail[64] = { 0 };
    char name_decoded[64] = { 0 };
    char subj_decoded[64] = { 0 };

	// Parse From
	if ((spos = strstr((char *)ptr, "\r\nFrom:")) != 0) {
        spos += 8; // "\r\nFrom: "
        epos = strstr(spos, "\r\n");

        char * email_start = strstr(spos, "<") + 1;
        if(email_start < epos) {
            char * email_end = strstr(email_start, ">");
            *email_end = 0;
            strncpy(mail, email_start, 64);
            *email_end = '>';

            char * name_start = spos;
            char * name_end = email_start - 2;
            char bak = *name_end;
            *name_end = 0;
            strncpy(name, name_start, 64);
            *name_end = bak;
        } else {
            // no name, just email
            char * name_start = spos;
            char * name_end = strstr(name_start, "\r\n");
            char bak = *name_end;
            *name_end = 0;
            strncpy(name, name_start, 64);
            *name_end = bak;
        }

        rfc2047_decode(name_decoded, name, 64);

        msg.sender_name = name_decoded;
        msg.sender_mail = mail;
		ESP_LOGI(LOG_TAG, "From: %s [%s]", msg.sender_name, msg.sender_mail);
	}

	// Parse Subject
	if ((spos = strstr((char *)ptr, "\r\nSubject:")) != 0) {
        spos += 11; // "\r\nSubject: "
		epos = strstr(spos, "\r\n");
        while(*(epos + 2) == ' ') {
            // Concat multi line subjects
            epos[0] = ' ';
            epos[1] = ' ';
            epos = strstr(epos, "\r\n");
        }
        *epos = 0;

        rfc2047_decode(subj_decoded, spos, 64);

		msg.subject = subj_decoded;
		ESP_LOGI(LOG_TAG, "Subj: [%s]", msg.subject);
	}

    if(callback != nullptr) {
        callback(id, &msg);
    }

    known_ids.insert(id);

	free(ptr);
}

void ImapNotifier::download_queued() {
    while(!download_queue.empty()) {
        imap_message_id_t id = download_queue.back();
        download_queue.pop_back();
        download_one(id);
    }
}

void ImapNotifier::process_line(const char * line) {
    ESP_LOGI(LOG_TAG, "Line: %s", line);

    if(strncmp(line, TXN_TAG, 5) == 0) {
        const char * untagged_line = &line[5];

        switch(state) {
            case AUTHING:
                if(untagged_line[0] == 'O' && untagged_line[1] == 'K') {
                    ESP_LOGI(LOG_TAG, "Auth succeeded");
                    send("SELECT INBOX");
                    state = SELECTING;
                } else {
                    ESP_LOGW(LOG_TAG, "Auth failed");
                    deinit();
                }
                break;

            case SELECTING:
                if(untagged_line[0] == 'O' && untagged_line[1] == 'K') {
                    ESP_LOGI(LOG_TAG, "Select succeeded");
                    state = IDLE;
                    manual_check();
                } else {
                    ESP_LOGW(LOG_TAG, "Select failed");
                    deinit();
                }
                break;

            case CHECKING:
                if(untagged_line[0] == 'O' && untagged_line[1] == 'K' && untagged_line[3] == 'S') {
                    ESP_LOGI(LOG_TAG, "SEARCH succeeded");
                    download_queued();
                    if(idle_supported) {
                        send("IDLE");
                    }
                    state = IDLE;
                } else if(untagged_line[0] != 'O' && untagged_line[1] != 'K') {
                    ESP_LOGW(LOG_TAG, "Search failed");
                    deinit();
                }
                break;

            default:
                ESP_LOGW(LOG_TAG, "Did not expect any tagged lines in state %i", state);
                break;
        }
    } else if (line[0] == '*') {
        if(strncmp(line, "* CAPABILITY", 12) == 0) {
            idle_supported = strstr(line, "IDLE") != nullptr;
            ESP_LOGI(LOG_TAG, "Idle %s", idle_supported ? "supported" : "UNSUPPORTED");
        } else if(strncmp(line, "* SEARCH", 8) == 0 && state == CHECKING) {
            if(line[8] == 0) {
                ESP_LOGI(LOG_TAG, "Empty search result");
            } else {
                const char * rslt_str = &line[9];
                size_t len = strlen(rslt_str) + 1;

                char * work = (char*) malloc(len);
                if(work == nullptr) {
                    ESP_LOGE(LOG_TAG, "Out of memory when processing search result");
                    return;
                }

                strcpy(work, rslt_str);

                ESP_LOGI(LOG_TAG, "Got search results: %s", rslt_str);
                const char * idx_str = strtok(work, " ");
                while(idx_str != nullptr) {
                    imap_message_id_t id = atoi(idx_str);
                    ESP_LOGV(LOG_TAG, "ID = %i", id);
                    download_queue.push_back(id);
                    idx_str = strtok(nullptr, " ");
                }

                free(work);
            }
        } else if(state == IDLE) { 
            bool expunge = (strstr(line, "EXPUNGE") != nullptr);
            if(strstr(line, "EXISTS") != nullptr) {
                if(!did_just_expunge) {
                    ESP_LOGI(LOG_TAG, "New mail?");
                    // Not a command: cannot use send()
                    mbedtls_ssl_write(&ssl, (const unsigned char*)"DONE\r\n", 6);
                    manual_check();
                } else {
                    ESP_LOGI(LOG_TAG, "Ignoring EXISTS because of recent EXPUNGE");
                    did_just_expunge = false;
                }
            } else if(strstr(line, "FLAGS (\\Seen)") != nullptr || expunge) {
                const char * evt_str = &line[2];
                size_t len = strstr(evt_str, " ") - evt_str;

                char * id_str = (char*) malloc(len + 1);
                if(id_str == nullptr) {
                    ESP_LOGE(LOG_TAG, "Out of memory while processing event");
                    return;
                }

                memcpy(id_str, evt_str, len);
                id_str[len] = 0;

                imap_message_id_t id = atoi(id_str);

                if(known_ids.count(id) == 0) {
                    ESP_LOGW(LOG_TAG, "Tried to yeet msg %i but it's not something we know? Something is not right, we need to redownload messages.", id);
                    mbedtls_ssl_write(&ssl, (const unsigned char*)"DONE\r\n", 6);
                    manual_check();
                    return;
                }

                ESP_LOGI(LOG_TAG, "Yeet msg %i", id);
                if(callback != nullptr) {
                    callback(id, nullptr);
                }
                known_ids.erase(id);

                if(expunge) {
                    did_just_expunge = true;
                }
            }
        }
    }
}

#define BUF_SIZE 512

static int perform_tls_handshake(mbedtls_ssl_context *ssl)
{
	int ret = -1;
	uint32_t flags;
	char *buf = NULL;
	buf = (char *) calloc(1, BUF_SIZE);
	if (buf == NULL) {
		ESP_LOGE(LOG_TAG, "calloc failed for size %d", BUF_SIZE);
		goto exit;
	}

	ESP_LOGI(LOG_TAG, "Performing the SSL/TLS handshake...");

	fflush(stdout);
	while ((ret = mbedtls_ssl_handshake(ssl)) != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			ESP_LOGE(LOG_TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
			goto exit;
		}
	}

	ESP_LOGI(LOG_TAG, "Verifying peer X.509 certificate...");

	if ((flags = mbedtls_ssl_get_verify_result(ssl)) != 0) {
		/* In real life, we probably want to close connection if ret != 0 */
		ESP_LOGW(LOG_TAG, "Failed to verify peer certificate!");
		mbedtls_x509_crt_verify_info(buf, BUF_SIZE, "  ! ", flags);
		ESP_LOGW(LOG_TAG, "verification info: %s", buf);
	} else {
		ESP_LOGI(LOG_TAG, "Certificate verified.");
	}

	ESP_LOGI(LOG_TAG, "Cipher suite is %s", mbedtls_ssl_get_ciphersuite(ssl));
	ret = 0; /* No error */

exit:
	if (buf) {
		free(buf);
	}
	return ret;
}

void ImapNotifier::connect() {
    int ret;

    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_ssl_config_init(&conf);
    mbedtls_entropy_init(&entropy);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0) {
		ESP_LOGE(LOG_TAG, "mbedtls_ctr_drbg_seed returned -0x%x", -ret);
		tlserror = ret;
        deinit();
        return;
	}

    SPIFFS.begin();
    if(!SPIFFS.exists(PEM_FILE_NAME)) {
        ESP_LOGE(LOG_TAG, "%s does not exist in SPIFFS", PEM_FILE_NAME);
        deinit();
        return;
    }
    
    if(pem_content == nullptr) {
        File pem = SPIFFS.open(PEM_FILE_NAME);
        pem_size = pem.size() + 1;
        pem_content = (uint8_t*) malloc(pem_size);
        if(pem_content == nullptr) {
            ESP_LOGE(LOG_TAG, "Out of memory when reading PEM!");
            deinit();
            return;
        }
        if(pem.readBytes((char*)pem_content, pem_size - 1) < pem_size - 1) {
            ESP_LOGE(LOG_TAG, "Incomplete read when reading PEM");
            free(pem_content);
            pem_content = nullptr;
            deinit();
            return;
        }
        pem_content[pem_size - 1] = 0; // null terminator
    }

    if ((ret = mbedtls_x509_crt_parse(&cacert, pem_content, pem_size)) != 0) {
		ESP_LOGE(LOG_TAG, "mbedtls_x509_crt_parse returned -0x%x", -ret);
		tlserror = ret;
        deinit();
        return;
	}

    if ((ret = mbedtls_ssl_set_hostname(&ssl, server)) != 0) {
		ESP_LOGE(LOG_TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
		tlserror = ret;
        deinit();
        return;
	}

    if ((ret = mbedtls_ssl_config_defaults(&conf,
										   MBEDTLS_SSL_IS_CLIENT,
										   MBEDTLS_SSL_TRANSPORT_STREAM,
										   MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
		ESP_LOGE(LOG_TAG, "mbedtls_ssl_config_defaults returned -0x%x", -ret);
		tlserror = ret;
        deinit();
        return;
	}

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
		ESP_LOGE(LOG_TAG, "mbedtls_ssl_setup returned -0x%x", -ret);
		tlserror = ret;
        deinit();
        return;
	}

	mbedtls_net_init(&server_fd);

	ESP_LOGI(LOG_TAG, "Connecting to %s:%s...", server, port);

    if ((ret = mbedtls_net_connect(&server_fd, server, port, MBEDTLS_NET_PROTO_TCP)) != 0) {
		ESP_LOGE(LOG_TAG, "mbedtls_net_connect returned -0x%x", -ret);
		tlserror = ret;
        deinit();
        return;
	}

	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

	if ((ret = perform_tls_handshake(&ssl)) != 0) { 
		ESP_LOGE(LOG_TAG, "perform_tls_handshake returned -0x%x", -ret);
        deinit();
        return;
	}
	ESP_LOGI(LOG_TAG, "Connected.");

    char buf[256] = { 0 };
    size_t len = mbedtls_ssl_read(&ssl, (unsigned char*) buf, 255);
    ESP_LOGV(LOG_TAG, "IDENT: %s", buf);
    if(strncmp("* OK", buf, 4) != 0) {
        ESP_LOGE(LOG_TAG, "Unexpected response from server");
        deinit();
        return;
    }

    state = CONNECT;
}

void ImapNotifier::send(const char* tx) {
    mbedtls_ssl_write(&ssl, (const unsigned char*)TXN_TAG, 5);
    mbedtls_ssl_write(&ssl, (const unsigned char*)tx, strlen(tx));
    ESP_LOGV(LOG_TAG, "Tx: %s", tx);
    mbedtls_ssl_write(&ssl, (const unsigned char*)"\r\n", 2);
}

void ImapNotifier::login() {
    char txbuf[256] = { 0 };
    snprintf(txbuf, 255, "LOGIN %s %s", username, password);
    send(txbuf);
    state = AUTHING;
}

void ImapNotifier::free_socket() {
    if(tlserror != 0) {
        char buf[100];
        mbedtls_strerror(tlserror, buf, 100);
        ESP_LOGE(LOG_TAG, "TLS error -0x%x: %s", -tlserror, buf);
    }

    mbedtls_net_free(&server_fd);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}

void ImapNotifier::deinit() {
    free_socket();

    if(hTask != NULL) {
        vTaskDelete(hTask);
    }

    state = DISCONNECT;
}

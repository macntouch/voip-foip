void sccp_init_device(sccp_device_t * d);

void sccp_handle_alarm(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_register(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_line_number(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_speed_dial_stat_req(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_stimulus(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_offhook(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_onhook(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_headset(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_capabilities_res(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_button_template_req(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_soft_key_template_req(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_soft_key_set_req(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_time_date_req(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_keypad_button(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_soft_key_event(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_open_recieve_channel_ack(sccp_session_t * s, sccp_moo_t * r);
void sccp_handle_version(sccp_session_t * s, sccp_moo_t * r);


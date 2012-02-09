
/*
 * Copyright (C) Maxim Dounin
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_msec_t                   delay;
} ngx_http_delay_conf_t;


static void ngx_http_delay_event_handler(ngx_http_request_t *r);


static void *ngx_http_delay_create_conf(ngx_conf_t *cf);
static char *ngx_http_delay_merge_conf(ngx_conf_t *cf, void *parent,
    void *child);
static ngx_int_t ngx_http_delay_init(ngx_conf_t *cf);


static ngx_command_t  ngx_http_delay_commands[] = {

    { ngx_string("delay"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_delay_conf_t, delay),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_delay_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_delay_init,                   /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_delay_create_conf,            /* create location configration */
    ngx_http_delay_merge_conf              /* merge location configration */
};


ngx_module_t  ngx_http_delay_module = {
    NGX_MODULE_V1,
    &ngx_http_delay_module_ctx,            /* module context */
    ngx_http_delay_commands,               /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_delay_handler(ngx_http_request_t *r)
{
    ngx_http_delay_conf_t  *dcf;

    if (ngx_http_get_module_ctx(r, ngx_http_delay_module) != NULL) {
        return NGX_DECLINED;
    }

    dcf = ngx_http_get_module_loc_conf(r, ngx_http_delay_module);

    if (dcf->delay == NGX_CONF_UNSET_MSEC) {
        return NGX_DECLINED;
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "delaying request");

    if (ngx_handle_read_event(r->connection->read, 0) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    r->read_event_handler = ngx_http_test_reading;
    r->write_event_handler = ngx_http_delay_event_handler;

    ngx_add_timer(r->connection->write, dcf->delay);

    ngx_http_set_ctx(r, (void *) 1, ngx_http_delay_module);

    return NGX_AGAIN;
}


static void
ngx_http_delay_event_handler(ngx_http_request_t *r)
{
    ngx_event_t  *wev;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "delay");

    wev = r->connection->write;

    if (!wev->timedout) {

        if (ngx_handle_write_event(wev, 0) != NGX_OK) {
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        }

        return;
    }

    wev->timedout = 0;

    if (ngx_handle_read_event(r->connection->read, 0) != NGX_OK) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    r->read_event_handler = ngx_http_block_reading;
    r->write_event_handler = ngx_http_core_run_phases;

    ngx_http_core_run_phases(r);
}


static void *
ngx_http_delay_create_conf(ngx_conf_t *cf)
{
    ngx_http_delay_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_delay_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     */

    conf->delay = NGX_CONF_UNSET_MSEC;

    return conf;
}


static char *
ngx_http_delay_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_delay_conf_t *prev = parent;
    ngx_http_delay_conf_t *conf = child;

    ngx_conf_merge_msec_value(conf->delay, prev->delay,
                              NGX_CONF_UNSET_MSEC);

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_delay_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_PREACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_delay_handler;

    return NGX_OK;
}

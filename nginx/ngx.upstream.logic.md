#upstream 模块

* [module](#module)
    * [ngx_http_upstream_module_ctx](#ngx_http_upstream_module_ctx)
    * [ngx_http_upstream_main_conf_t](#ngx_http_upstream_main_conf_t)
    * [ngx_http_upstream_srv_conf_t](#ngx_http_upstream_srv_conf_t)
    * [ngx_http_upstream_create_main_conf](#ngx_http_upstream_create_main_conf)
    * [ngx_http_upstream_init_main_conf](#ngx_http_upstream_init_main_conf)

* [data struct](#data-struct)
    * [ngx_http_upstream_peer_t](#ngx_http_upstream_peer_t)
    * [ngx_http_upstream_conf_t](#ngx_http_upstream_conf_t)
    * [ngx_http_upstream_server_t](#ngx_http_upstream_server_t)
    * [ngx_http_upstream_s](#ngx_http_upstream_s)

* [ngx_http_upstream_commands](#ngx_http_upstream_commands)
    * [upstream_command](#upstream_command)
        * [ngx_http_upstream](#ngx_http_upstream)
    * [server_command](#server_command)
        * [ngx_http_upstream_server](#ngx_http_upstream_server)


<span id="module"></span>
###1 module

<span id="ngx_http_upstream_module_ctx"></span>
####1.1 ngx_http_upstream_module_ctx
ngx_http_upstream_create_main_conf


```c
static ngx_http_module_t  ngx_http_upstream_module_ctx = {
    ngx_http_upstream_add_variables,       /* preconfiguration */
    NULL,                                  /* postconfiguration */

    ngx_http_upstream_create_main_conf,    /* create main configuration */
    ngx_http_upstream_init_main_conf,      /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};

```


<span id="ngx_http_upstream_main_conf_t"></span>
####1.2 ngx_http_upstream_main_conf_t
```c
typedef struct {
    ngx_hash_t                       headers_in_hash;
    //保存了所有的 upstream 配置项ngx_http_upstream_srv_conf_t
    ngx_array_t                      upstreams;  
} ngx_http_upstream_main_conf_t;
```


<span id="ngx_http_upstream_srv_conf_t"></span>
####1.3 ngx_http_upstream_srv_conf_t 
>upstream module svr级别的配置   
>保存了 proxy_pass host、port参数   
>关联一组ngx_http_upstream_server_t(server 指令)   
>在proxy模块会根据proxy_pass 指令找到 被关联到ngx_http_upstream_conf_t

```c
struct ngx_http_upstream_srv_conf_s {  
    ngx_http_upstream_peer_t         peer;
    void                           **srv_conf;

    ngx_array_t                     *servers;  /* ngx_http_upstream_server_t upstream 中的server*/

    ngx_uint_t                       flags;//NGX_HTTP_UPSTREAM_CREATE
    //host port应该是 proxy_pass 后面带的参数
    ngx_str_t                        host; 
    u_char                          *file_name;
    ngx_uint_t                       line;
    in_port_t                        port;
    in_port_t                        default_port;
    ngx_uint_t                       no_port;  /* unsigned no_port:1 */

#if (NGX_HTTP_UPSTREAM_ZONE)
    ngx_shm_zone_t                  *shm_zone;
#endif
};
```

<span id="ngx_http_upstream_create_main_conf"></span>
####1.4 ngx_http_upstream_create_main_conf(ngx_conf_t *cf)

>创建 ngx_http_upstream_main_conf_t



<span id="ngx_http_upstream_init_main_conf"></span>
####1.5 ngx_http_upstream_init_main_conf(ngx_conf_t *cf, void *conf)
>取出 ngx_http_upstream_srv_conf_t数组   
>获取每个元素的peer成员的init_upstream 方法并调用,如果不存在默认ngx_http_upstream_init_round_robin


```c
static char *
ngx_http_upstream_init_main_conf(ngx_conf_t *cf, void *conf)
{
    ngx_http_upstream_main_conf_t  *umcf = conf;
    ngx_http_upstream_srv_conf_t  **uscfp;
    uscfp = umcf->upstreams.elts;
    //设置 peer的init_upstream
    for (i = 0; i < umcf->upstreams.nelts; i++) {

        init = uscfp[i]->peer.init_upstream ? uscfp[i]->peer.init_upstream:
                                            ngx_http_upstream_init_round_robin;
        if (init(cf, uscfp[i]) != NGX_OK) {
            return NGX_CONF_ERROR;
        }
    }
    return NGX_CONF_OK;
}
```
<span id="data-struct"></span>
###2 data struct




<span id="ngx_http_upstream_peer_t"></span>
####2.2 ngx_http_upstream_peer_t 

```c 
typedef struct {
    ngx_http_upstream_init_pt        init_upstream;  // ngx_http_upstream_init_round_robin
    //init_upstream中设置ngx_http_upstream_init_round_robin_peer
    ngx_http_upstream_init_peer_pt   init;

    void                            *data;// 保存按一定策略分配好的后端服务器
} ngx_http_upstream_peer_t;

```


<span id="ngx_http_upstream_conf_t"></span>
####2.3 ngx_http_upstream_conf_t 
>用于保存 location下 proxy 相关的参数。

```c

typedef struct {
    ngx_http_upstream_srv_conf_t    *upstream;

    ngx_msec_t                       connect_timeout;
    ngx_msec_t                       send_timeout;  //tcp 超时参数
    ngx_msec_t                       read_timeout;
    ngx_msec_t                       timeout;
    ngx_msec_t                       next_upstream_timeout;

    size_t                           send_lowat;
    size_t                           buffer_size;
    size_t                           limit_rate;

    size_t                           busy_buffers_size;
    size_t                           max_temp_file_size;
    size_t                           temp_file_write_size;

    size_t                           busy_buffers_size_conf;
    size_t                           max_temp_file_size_conf;
    size_t                           temp_file_write_size_conf;

    ngx_bufs_t                       bufs;

    ngx_uint_t                       ignore_headers;
    ngx_uint_t                       next_upstream;
    ngx_uint_t                       store_access;
    ngx_uint_t                       next_upstream_tries;
    ngx_flag_t                       buffering;
    ngx_flag_t                       request_buffering;
    ngx_flag_t                       pass_request_headers;
    ngx_flag_t                       pass_request_body;

    ngx_flag_t                       ignore_client_abort;
    ngx_flag_t                       intercept_errors;
    ngx_flag_t                       cyclic_temp_file;
    ngx_flag_t                       force_ranges;

    ngx_path_t                      *temp_path;

    ngx_hash_t                       hide_headers_hash;
    ngx_array_t                     *hide_headers;
    ngx_array_t                     *pass_headers;

    ngx_http_upstream_local_t       *local;

#if (NGX_HTTP_CACHE)
    ngx_shm_zone_t                  *cache_zone;
    ngx_http_complex_value_t        *cache_value;

    ngx_uint_t                       cache_min_uses;
    ngx_uint_t                       cache_use_stale;
    ngx_uint_t                       cache_methods;

    ngx_flag_t                       cache_lock;
    ngx_msec_t                       cache_lock_timeout;
    ngx_msec_t                       cache_lock_age;

    ngx_flag_t                       cache_revalidate;
    ngx_flag_t                       cache_convert_head;

    ngx_array_t                     *cache_valid;
    ngx_array_t                     *cache_bypass;
    ngx_array_t                     *no_cache;
#endif

    ngx_array_t                     *store_lengths;
    ngx_array_t                     *store_values;

#if (NGX_HTTP_CACHE)
    signed                           cache:2;
#endif
    signed                           store:2;
    unsigned                         intercept_404:1;
    unsigned                         change_buffering:1;

#if (NGX_HTTP_SSL)
    ngx_ssl_t                       *ssl;
    ngx_flag_t                       ssl_session_reuse;

    ngx_http_complex_value_t        *ssl_name;
    ngx_flag_t                       ssl_server_name;
    ngx_flag_t                       ssl_verify;
#endif

    ngx_str_t                        module;
} ngx_http_upstream_conf_t;  

```

<span id="ngx_http_upstream_server_t"></span>
####2.4 ngx_http_upstream_server_t :
>upstream中的一个server   
>保存 weight、backup等信息
```c
typedef struct {      //upstream中的一个server
    ngx_str_t                        name;
    ngx_addr_t                      *addrs; //同一个域名解析出来多个ip
    ngx_uint_t                       naddrs; //ip格式
    ngx_uint_t                       weight;
    ngx_uint_t                       max_fails;
    time_t                           fail_timeout;

    unsigned                         down:1; 
    unsigned                         backup:1;
} ngx_http_upstream_server_t;

```

<span id="ngx_http_upstream_s"></span>
#####2.5 ngx_http_upstream_s 


```c

struct ngx_http_upstream_s {//request 中的成员
    ngx_http_upstream_handler_pt     read_event_handler;
    ngx_http_upstream_handler_pt     write_event_handler;

    ngx_peer_connection_t            peer;

    ngx_event_pipe_t                *pipe;

    ngx_chain_t                     *request_bufs;

    ngx_output_chain_ctx_t           output;
    ngx_chain_writer_ctx_t           writer;

    ngx_http_upstream_conf_t        *conf;
#if (NGX_HTTP_CACHE)
    ngx_array_t                     *caches;
#endif

    ngx_http_upstream_headers_in_t   headers_in;

    ngx_http_upstream_resolved_t    *resolved;

    ngx_buf_t                        from_client;

    ngx_buf_t                        buffer;
    off_t                            length;

    ngx_chain_t                     *out_bufs;
    ngx_chain_t                     *busy_bufs;
    ngx_chain_t                     *free_bufs;

    ngx_int_t                      (*input_filter_init)(void *data);
    ngx_int_t                      (*input_filter)(void *data, ssize_t bytes);
    void                            *input_filter_ctx;

#if (NGX_HTTP_CACHE)
    ngx_int_t                      (*create_key)(ngx_http_request_t *r);
#endif
    ngx_int_t                      (*create_request)(ngx_http_request_t *r);
    ngx_int_t                      (*reinit_request)(ngx_http_request_t *r);
    ngx_int_t                      (*process_header)(ngx_http_request_t *r);
    void                           (*abort_request)(ngx_http_request_t *r);
    void                           (*finalize_request)(ngx_http_request_t *r,
                                         ngx_int_t rc);
    ngx_int_t                      (*rewrite_redirect)(ngx_http_request_t *r,
                                         ngx_table_elt_t *h, size_t prefix);
    ngx_int_t                      (*rewrite_cookie)(ngx_http_request_t *r,
                                         ngx_table_elt_t *h);

    ngx_msec_t                       timeout;

    ngx_http_upstream_state_t       *state;

    ngx_str_t                        method;
    ngx_str_t                        schema;
    ngx_str_t                        uri;

#if (NGX_HTTP_SSL)
    ngx_str_t                        ssl_name;
#endif

    ngx_http_cleanup_pt             *cleanup;

    unsigned                         store:1;
    unsigned                         cacheable:1;
    unsigned                         accel:1;
    unsigned                         ssl:1;
#if (NGX_HTTP_CACHE)
    unsigned                         cache_status:3;
#endif

    unsigned                         buffering:1;
    unsigned                         keepalive:1;
    unsigned                         upgrade:1;

    unsigned                         request_sent:1;
    unsigned                         header_sent:1;
};

```
<span id="ngx_http_upstream_commands"></span>
###2 ngx_http_upstream_commands 
```c
static ngx_command_t  ngx_http_upstream_commands[] = {

    { ngx_string("upstream"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_TAKE1,
      ngx_http_upstream,
      0,
      0,
      NULL },

    { ngx_string("server"),
      NGX_HTTP_UPS_CONF|NGX_CONF_1MORE,
      ngx_http_upstream_server,
      NGX_HTTP_SRV_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};
```
<span id="upstream-command"></span>

####3.1 upstream

ngx_http_upstream(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy)
>创建 ngx_http_upstream_srv_conf_t

<span id="ngx_http_upstream"></span>
```c
static char *
ngx_http_upstream(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy)
{
    char                          *rv;
    void                          *mconf;
    ngx_str_t                     *value;
    ngx_url_t                      u;
    ngx_uint_t                     m;
    ngx_conf_t                     pcf;
    ngx_http_module_t             *module;
    ngx_http_conf_ctx_t           *ctx, *http_ctx;
    ngx_http_upstream_srv_conf_t  *uscf;

    ngx_memzero(&u, sizeof(ngx_url_t));

    value = cf->args->elts;
    u.host = value[1];
    u.no_resolve = 1;
    u.no_port = 1;
    //这里只取了第一个参数，用于初始化 ngx_http_upstream_srv_conf_t 里面的server数组，后续其他参数在 ngx_http_upstream_server 里面处理
    //ngx_http_upstream_srv_conf_t 里面的host port 也在这里设置，应该是 proxy_pass 后面带的参数
    uscf = ngx_http_upstream_add(cf, &u, NGX_HTTP_UPSTREAM_CREATE
                                         |NGX_HTTP_UPSTREAM_WEIGHT
                                         |NGX_HTTP_UPSTREAM_MAX_FAILS
                                         |NGX_HTTP_UPSTREAM_FAIL_TIMEOUT
                                         |NGX_HTTP_UPSTREAM_DOWN
                                         |NGX_HTTP_UPSTREAM_BACKUP);//创建ngx_http_upstream_srv_conf_t
    if (uscf == NULL) {
        return NGX_CONF_ERROR;
    }


    ctx = ngx_pcalloc(cf->pool, sizeof(ngx_http_conf_ctx_t));
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    http_ctx = cf->ctx;  //cf:ngx_conf_t
    ctx->main_conf = http_ctx->main_conf;

    /* the upstream{}'s srv_conf */

    ctx->srv_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_http_max_module);
    if (ctx->srv_conf == NULL) {
        return NGX_CONF_ERROR;
    }

    ctx->srv_conf[ngx_http_upstream_module.ctx_index] = uscf;

    uscf->srv_conf = ctx->srv_conf;


    /* the upstream{}'s loc_conf */

    ctx->loc_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_http_max_module);
    if (ctx->loc_conf == NULL) {
        return NGX_CONF_ERROR;
    }

    for (m = 0; ngx_modules[m]; m++) {
        if (ngx_modules[m]->type != NGX_HTTP_MODULE) {
            continue;
        }

        module = ngx_modules[m]->ctx;

        if (module->create_srv_conf) {// upstream 模块 该方法为NULL ，所以不会覆盖
            mconf = module->create_srv_conf(cf);
            if (mconf == NULL) {
                return NGX_CONF_ERROR;
            }

            ctx->srv_conf[ngx_modules[m]->ctx_index] = mconf;
        }

        if (module->create_loc_conf) {
            mconf = module->create_loc_conf(cf);
            if (mconf == NULL) {
                return NGX_CONF_ERROR;
            }

            ctx->loc_conf[ngx_modules[m]->ctx_index] = mconf;
        }
    }

    uscf->servers = ngx_array_create(cf->pool, 4,
                                     sizeof(ngx_http_upstream_server_t));
    if (uscf->servers == NULL) {
        return NGX_CONF_ERROR;
    }


    /* parse inside upstream{} */

    pcf = *cf;
    cf->ctx = ctx;// ctx 就是 ngx_http_conf_ctx_t
    cf->cmd_type = NGX_HTTP_UPS_CONF;

    rv = ngx_conf_parse(cf, NULL);
    *cf = pcf;
    if (rv != NGX_CONF_OK) {
        return rv;
    }

    if (uscf->servers->nelts == 0) {//必须要有server 难怪 lua balancer插件 必须写一个server
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "no servers are inside upstream");
        return NGX_CONF_ERROR;
    }

    return rv;
}
```
<span id="server-command"></span>
####3.2 server

>读取 upstream 里面的server的信息 保存在 ngx_http_upstream_server_t   
>将ngx_http_upstream_server_t保存在 ngx_http_upstream_srv_conf_t 的servers里面


<span id="ngx_http_upstream_server"></span>

```c

static char *
ngx_http_upstream_server(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_upstream_srv_conf_t  *uscf = conf;

    time_t                       fail_timeout;
    ngx_str_t                   *value, s;
    ngx_url_t                    u;
    ngx_int_t                    weight, max_fails;
    ngx_uint_t                   i;
    ngx_http_upstream_server_t  *us;

    us = ngx_array_push(uscf->servers);//创建一个server:ngx_http_upstream_server_t
    if (us == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_memzero(us, sizeof(ngx_http_upstream_server_t));

    value = cf->args->elts;

    weight = 1;
    max_fails = 1;
    fail_timeout = 10;

    for (i = 2; i < cf->args->nelts; i++) {//对server逐个赋值

        if (ngx_strncmp(value[i].data, "weight=", 7) == 0) {

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_WEIGHT)) {
                goto not_supported;
            }

            weight = ngx_atoi(&value[i].data[7], value[i].len - 7);

            if (weight == NGX_ERROR || weight == 0) {
                goto invalid;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "max_fails=", 10) == 0) {

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_MAX_FAILS)) {
                goto not_supported;
            }

            max_fails = ngx_atoi(&value[i].data[10], value[i].len - 10);

            if (max_fails == NGX_ERROR) {
                goto invalid;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "fail_timeout=", 13) == 0) {

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_FAIL_TIMEOUT)) {
                goto not_supported;
            }

            s.len = value[i].len - 13;
            s.data = &value[i].data[13];

            fail_timeout = ngx_parse_time(&s, 1);

            if (fail_timeout == (time_t) NGX_ERROR) {
                goto invalid;
            }

            continue;
        }

        if (ngx_strcmp(value[i].data, "backup") == 0) {

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_BACKUP)) {
                goto not_supported;
            }

            us->backup = 1;

            continue;
        }

        if (ngx_strcmp(value[i].data, "down") == 0) {

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_DOWN)) {
                goto not_supported;
            }

            us->down = 1;

            continue;
        }

        goto invalid;
    }

    ngx_memzero(&u, sizeof(ngx_url_t));

    u.url = value[1];
    u.default_port = 80;

    if (ngx_parse_url(cf->pool, &u) != NGX_OK) {
        if (u.err) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "%s in upstream \"%V\"", u.err, &u.url);
        }

        return NGX_CONF_ERROR;
    }

    us->name = u.url;
    us->addrs = u.addrs;
    us->naddrs = u.naddrs;
    us->weight = weight;
    us->max_fails = max_fails;
    us->fail_timeout = fail_timeout;

    return NGX_CONF_OK;

invalid:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "invalid parameter \"%V\"", &value[i]);
    return NGX_CONF_ERROR;

not_supported:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "balancing method does not support parameter \"%V\"",
                       &value[i]);

    return NGX_CONF_ERROR;
}
```



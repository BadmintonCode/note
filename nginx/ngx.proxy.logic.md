#proxy 模块


* [module](#module)
    * [ngx_http_proxy_module_ctx](#ngx_http_proxy_module_ctx)
    * [ngx_http_proxy_create_main_conf](#ngx_http_proxy_create_main_conf)
    * [ngx_http_proxy_create_loc_conf](#ngx_http_proxy_create_loc_conf)
* [data struct](#data-struct)
    * [ngx_http_proxy_ctx_t](#ngx_http_proxy_ctx_t)
    * [ngx_http_proxy_loc_conf_t](#ngx_http_proxy_loc_conf_t)
* [command](#command)
    * [proxy_pass command](#proxy_pass-command)
        * [ngx_http_proxy_pass](#ngx_http_proxy_pass)
        * [ngx_http_proxy_handler](#ngx_http_proxy_handler)
    * [proxy_connect_timeout command](#proxy_connect_timeout-command)

<span id="module"></span>
###1 module 
<span id="ngx_http_proxy_module_ctx"></span>
####1.1 ngx_http_proxy_module_ctx

>svr 配置函数为NULL

```c
static ngx_http_module_t  ngx_http_proxy_module_ctx = {
    ngx_http_proxy_add_variables,          /* preconfiguration */
    NULL,                                  /* postconfiguration */

    ngx_http_proxy_create_main_conf,       /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_proxy_create_loc_conf,        /* create location configuration */
    ngx_http_proxy_merge_loc_conf          /* merge location configuration */
};

```
<span id="ngx_http_proxy_create_main_conf"></span>
####1.2 ngx_http_proxy_create_main_conf

>main级别 

static void *ngx_http_proxy_create_main_conf(ngx_conf_t *cf);

>创建ngx_http_proxy_create_main_conf

```c
typedef struct {
    ngx_array_t                    caches;  /* ngx_http_file_cache_t * */
} ngx_http_proxy_main_conf_t;
```
<span id="ngx_http_proxy_create_loc_conf"></span>
####1.3 ngx_http_proxy_create_loc_conf

>svr 级别

static void *ngx_http_proxy_create_loc_conf(ngx_conf_t *cf);

>创建 ngx_http_proxy_loc_conf_t ，并设置upstream(类型 ngx_http_upstream_conf_t) 成员相关参数。

```c
    conf->upstream.connect_timeout = NGX_CONF_UNSET_MSEC; //conf 类型为 ngx_http_proxy_loc_conf_t
    conf->upstream.send_timeout = NGX_CONF_UNSET_MSEC;
    conf->upstream.read_timeout = NGX_CONF_UNSET_MSEC;
```

<span id="data-struct"></span>

###2 data struct

<span id="ngx_http_proxy_ctx_t"></span>
#### 2.1 ngx_http_proxy_ctx_t 
>上下文
```c
typedef struct {
    ngx_http_status_t              status;
    ngx_http_chunked_t             chunked;
    ngx_http_proxy_vars_t          vars;
    off_t                          internal_body_length;

    ngx_chain_t                   *free;
    ngx_chain_t                   *busy;

    unsigned                       head:1;
    unsigned                       internal_chunked:1;
    unsigned                       header_sent:1;
} ngx_http_proxy_ctx_t;
```
<span id="ngx_http_proxy_loc_conf_t"></span>
#### 2.2 ngx_http_proxy_loc_conf_t
>proxy loc 配置

```c
typedef struct {
    ngx_http_upstream_conf_t       upstream;  //

    ngx_array_t                   *body_flushes;
    ngx_array_t                   *body_lengths;
    ngx_array_t                   *body_values;
    ngx_str_t                      body_source;

    ngx_http_proxy_headers_t       headers;
#if (NGX_HTTP_CACHE)
    ngx_http_proxy_headers_t       headers_cache;
#endif
    ngx_array_t                   *headers_source;

    ngx_array_t                   *proxy_lengths;
    ngx_array_t                   *proxy_values;

    ngx_array_t                   *redirects;
    ngx_array_t                   *cookie_domains;
    ngx_array_t                   *cookie_paths;

    ngx_str_t                      method;
    ngx_str_t                      location;
    ngx_str_t                      url;

#if (NGX_HTTP_CACHE)
    ngx_http_complex_value_t       cache_key;
#endif

    ngx_http_proxy_vars_t          vars;

    ngx_flag_t                     redirect;

    ngx_uint_t                     http_version;

    ngx_uint_t                     headers_hash_max_size;
    ngx_uint_t                     headers_hash_bucket_size;

#if (NGX_HTTP_SSL)
    ngx_uint_t                     ssl;
    ngx_uint_t                     ssl_protocols;
    ngx_str_t                      ssl_ciphers;
    ngx_uint_t                     ssl_verify_depth;
    ngx_str_t                      ssl_trusted_certificate;
    ngx_str_t                      ssl_crl;
    ngx_str_t                      ssl_certificate;
    ngx_str_t                      ssl_certificate_key;
    ngx_array_t                   *ssl_passwords;
#endif
} ngx_http_proxy_loc_conf_t;
```




<span id="command"></span>

###3 command 

<span id="proxy_pass-command"></span>

####3.1 "proxy_pass" command

###### 定义

```c
    { ngx_string("proxy_pass"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_TAKE1,
      ngx_http_proxy_pass,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL }

```
<span id="ngx_http_proxy_pass"></span>
###### ngx_http_proxy_pass(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
>创建ngx_http_proxy_loc_conf_t 的成员ngx_http_upstream_conf_t  
>设置location下的各种参数到 ngx_http_upstream_conf_t  
>根据proxy_pass 后面的参数找到ngx_http_upstream_srv_conf_s 并关联到 ngx_http_upstream_conf_t  

```c
    ngx_http_proxy_loc_conf_t *plcf = conf;
    ngx_http_core_loc_conf_t *clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    //获取location 对应的 ngx_http_core_loc_conf_t
    clcf->handler = ngx_http_proxy_handler;
    //设置handler
    ngx_url_t u;
    u.url.len = url->len - add;
    u.url.data = url->data + add;
    u.default_port = port;
    //构造u

    //[ngx_http_proxy_loc_conf_t*]->[ngx_http_upstream_conf_t*].[ngx_http_upstream_srv_conf_s*]
    //找到url匹配的ngx_http_upstream_srv_conf_t(保存了proxy相关参数，upstream成员指向具体的upstream)
    plcf->upstream.upstream = ngx_http_upstream_add(cf, &u, 0);


    plcf->location = clcf->name;// location


```
<span id="ngx_http_proxy_handler"></span>
###### static ngx_int_t ngx_http_proxy_handler(ngx_http_request_t *r)

```c
    ngx_http_upstream_t         *u;
    //为request创建upstream(ngx_http_upstream_t)
    //既然是代理模块，必须是 proxy负责创建 upstream字段
    if (ngx_http_upstream_create(r) != NGX_OK) {  
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    //设置 request 上下文
    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_proxy_ctx_t));
    if (ctx == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ngx_http_set_ctx(r, ctx, ngx_http_proxy_module);

    u = r->upstream;
    //r->upstream->conf  
    //[ngx_http_request_s*]->[ngx_http_upstream_s*]->[ngx_http_upstream_conf_t*]
    //&plcf->upstream
    //[ngx_http_proxy_loc_conf_t*]->[ngx_http_upstream_conf_t*]
    u->conf = &plcf->upstream;//ngx_http_proxy_pass() 里面有设置 ngx_http_upstream_conf_t


    u->create_request = ngx_http_proxy_create_request;
    u->reinit_request = ngx_http_proxy_reinit_request;
    u->process_header = ngx_http_proxy_process_status_line;
    u->abort_request = ngx_http_proxy_abort_request;
    u->finalize_request = ngx_http_proxy_finalize_request;


    u->input_filter_init = ngx_http_proxy_input_filter_init;
    u->input_filter = ngx_http_proxy_non_buffered_copy_filter;
    u->input_filter_ctx = r;

    rc = ngx_http_read_client_request_body(r, ngx_http_upstream_init);

```
<span id="proxy_connect_timeout-command"></span>
####3.2 proxy_connect_timeout command

定义如下，主要用来设置 ngx_http_proxy_loc_conf_t 的upstream成员

```c
    { ngx_string("proxy_connect_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.connect_timeout),
      NULL },
```


/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * The strerror() messages are copied because:
 *
 * 1) strerror() and strerror_r() functions are not Async-Signal-Safe,
 *    therefore, they cannot be used in signal handlers;
 *
 * 2) a direct sys_errlist[] array may be used instead of these functions,
 *    but Linux linker warns about its usage:
 *
 * warning: `sys_errlist' is deprecated; use `strerror' or `strerror_r' instead
 * warning: `sys_nerr' is deprecated; use `strerror' or `strerror_r' instead
 *
 *    causing false bug reports.
 */

// nginx string errno 使用静态指针指向的数组 存储各种错误码的 string 解释
static ngx_str_t  *ngx_sys_errlist;
static ngx_str_t   ngx_unknown_error = ngx_string("Unknown error");


// 将错误码转化为相对应的 字符串解释，限制了最大字符串长度为 size
u_char *
ngx_strerror(ngx_err_t err, u_char *errstr, size_t size)
{
    ngx_str_t  *msg;
    // 错误码是否有效
    msg = ((ngx_uint_t) err < NGX_SYS_NERR) ? &ngx_sys_errlist[err]:
                                              &ngx_unknown_error;
    size = ngx_min(size, msg->len);
    // 底层使用 memcpy 完成字符创的复制
    return ngx_cpymem(errstr, msg->data, size);
}


// nginx string-errno 初始化
ngx_int_t
ngx_strerror_init(void)
{
    char       *msg;
    u_char     *p;
    size_t      len;
    ngx_err_t   err;

    /*
     * ngx_strerror() is not ready to work at this stage, therefore,
     * malloc() is used and possible errors are logged using strerror().
     */

    len = NGX_SYS_NERR * sizeof(ngx_str_t);

    // 开辟 len 长度的内存
    ngx_sys_errlist = malloc(len);
    if (ngx_sys_errlist == NULL) {
        goto failed;
    }

    for (err = 0; err < NGX_SYS_NERR; err++) {
        /* Return a string describing the meaning of the `errno' code in ERRNUM.  */
        // 获得系统中 errcode 对应的字符串
        msg = strerror(err);        // 得到 errno 对应的字符串
        len = ngx_strlen(msg);      // 获得字符串 长度

        p = malloc(len);
        if (p == NULL) {
            goto failed;
        }

        ngx_memcpy(p, msg, len);
        ngx_sys_errlist[err].len = len;
        ngx_sys_errlist[err].data = p;
    }

    return NGX_OK;

failed:

    err = errno;
    ngx_log_stderr(0, "malloc(%uz) failed (%d: %s)", len, err, strerror(err));

    return NGX_ERROR;
}

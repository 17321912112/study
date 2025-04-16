#ifndef _NGX_MEM_POOL_H
#define _NGX_MEM_POOL_H
/*移植nginx内存池代码，使用oop实现*/
#include <iostream>

using u_char = unsigned char;
using ngx_uint_t = unsigned int;
using uintptr_t = unsigned long;



struct ngx_pool_s;

/*大块内存头部信息*/
struct ngx_pool_large_s {
    ngx_pool_large_s     *next;     // 下一个大块内存信息
    void                 *alloc;    // 分配出去的内存地址
};

/*清理信息和回调*/
typedef void (*ngx_pool_cleanup_pt)(void *data);

struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler;  //清理函数
    void                 *data;     //回调函数参数
    ngx_pool_cleanup_s   *next;     //下一个清理信息
};

/*小块内存头部信息*/
struct ngx_pool_data_t
{
    u_char *last;                   // 可用内存起始地址
    u_char *end;                    // 可用内存结束地址
    ngx_pool_s *next;               // 下一个小块内存信息
    ngx_uint_t failed;              // 分配失败次数
};

struct ngx_pool_s {
    ngx_pool_data_t d;              //小块内存头部信息
    size_t max;                     //小块内存最大容量
    ngx_pool_s *current;            //第一块小块内存
    ngx_pool_large_s *large;        //大块内存入口地址
    ngx_pool_cleanup_s *cleanup;    //清理信息入口地址
};

#define NGX_ALIGNMENT   sizeof(unsigned long)           /* platform word */
const int NGX_PAGE_SIZE             = 4096;                  //默认页大小
const int NGX_MAX_ALLOC_FROM_POOL  = NGX_PAGE_SIZE - 1;      //小块内存最大容量

const int NGX_DEFAULT_POOL_SIZE    = 16 * 1024;             //默认16k内存池大小

const int NGX_POOL_ALIGNMENT       = 16;                    //内存按照16字节对齐

#define ngx_align(d, a)  (((d) + (a - 1)) & ~(a - 1))       // 调整到a的倍数

#define ngx_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

const int NGX_MIN_POOL_SIZE        = ngx_align((sizeof(ngx_pool_s) + 2 * sizeof(ngx_pool_large_s)),  NGX_POOL_ALIGNMENT); // 调整到NGX_POOL_ALIGNMENT最小邻近倍数

const ngx_uint_t ALIGN = 1;

const ngx_uint_t NOT_ALIGN = 0;


class ngx_mem_pool
{
public:
    bool ngx_create_pool(size_t size);                              //创建指定大小的内存池
    void *ngx_palloc(size_t size);                                  //向内存池申请指定大小的内存，考虑内存字节对齐
    void *ngx_pnalloc(size_t size);                                 //向内存池申请指定大小的内存，不考虑字节对齐
    void *ngx_pcalloc(size_t size);                                 //向内存池申请指定大小的内存，并把内容清零
    void  ngx_destroy_pool();                                       //销毁内存池
    void  ngx_reset_pool();                                         //重置内存池
    void  ngx_pfree(void *p);                                       //释放大块内存
    void  ngx_cleanup_add(ngx_pool_cleanup_pt handler, void *data); //添加清理信息
private:
    void *ngx_palloc_small(size_t size, ngx_uint_t align);         //小块内存分配
    void *ngx_palloc_large(size_t size);                            //大块内存分配
    void *ngx_palloc_block(size_t size);                            //新的小块内存分配
private:    
    ngx_pool_s *pool;                                       //内存池结构体
};

#endif
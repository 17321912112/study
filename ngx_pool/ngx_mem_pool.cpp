#include "ngx_mem_pool.h"
#include <cstring>


void *ngx_mem_pool::ngx_palloc_small(size_t size, ngx_uint_t align) 
{
    ngx_pool_s *p = pool->current;
    u_char      *m;
    
    for (p = pool->current; p ; p = p->d.next)
    {
        m = p->d.last;
        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }
        if (m + size <= p->d.end)
        {
            std::cout << "palloc small :" << size << std::endl;
            p->d.last = m + size;
            return m;
        }
    }
    return ngx_palloc_block(size);
}
void *ngx_mem_pool::ngx_palloc_large(size_t size)
{
    void *p = malloc(size);
    ngx_pool_large_s *large;
    // 遍历大块内存链表
    int n = 0;
    for (large = pool->large; large; large = large->next) {
        // 如果大块内存未分配，则分配内存并返回
        if (large->alloc == NULL) {
            large->alloc = p;
            std::cout << "palloc large :" << size << std::endl;
            return p;
        }

        // 防止无限循环，如果遍历超过3次则退出
        if (n++ > 3) {
            break;
        }
    }
    // 如果大块内存链表中没有可用的空间，则分配新的内存
    large = (ngx_pool_large_s*)ngx_palloc_small(sizeof(ngx_pool_large_s), 1);
    if (!large) {
        ngx_pfree(p);
        return nullptr;
    }
    large->alloc = p;
    large->next = pool->large;
    pool->large = large;
    std::cout << "palloc large :" << size << std::endl;
    return p;
}
void *ngx_mem_pool::ngx_palloc_block(size_t size)
{
    size_t psize = (size_t)(pool->d.end - (u_char *)pool);
    ngx_pool_s *newpool = (ngx_pool_s*)malloc(psize), *p;
    u_char *m;
    if (!newpool) return nullptr;
    m = (u_char*)newpool + sizeof(ngx_pool_data_t);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    newpool->d.last = m + size;
    newpool->d.end = (u_char*)newpool + psize;
    
    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }
    std::cout << "palloc block :" << size << std::endl;
    p->d.next = newpool;
    return m;
}

bool ngx_mem_pool::ngx_create_pool(size_t size) 
{
    size = size < NGX_MIN_POOL_SIZE ? NGX_MIN_POOL_SIZE : size;
    pool = (ngx_pool_s*)malloc(size);
    if (!pool) return false;
    size -= sizeof(ngx_pool_s);
    pool->d.last = (u_char*)pool + sizeof(ngx_pool_s);
    pool->d.end = (u_char*)pool + size;
    pool->d.next = NULL;
    pool->d.failed = 0;
    pool->max = size > NGX_MAX_ALLOC_FROM_POOL ? NGX_PAGE_SIZE : size;
    pool->current = pool;
    std::cout << "create pool size:" << size << std::endl;
    return true;
}
void *ngx_mem_pool::ngx_palloc(size_t size)
{
    if (size <= pool->max) return ngx_palloc_small(size, 1);
    else return ngx_palloc_large(size);
}
void *ngx_mem_pool::ngx_pnalloc(size_t size)
{
    if (size <= pool->max) return ngx_palloc_small(size, 0);
    else return ngx_palloc_large(size);
}
void *ngx_mem_pool::ngx_pcalloc(size_t size)
{
    void *p;
    if (size <= pool->max)  p = ngx_palloc_small(size, 0);
    else p = ngx_palloc_large(size);
    memset(p, 0, size);
    return p;
}
void  ngx_mem_pool::ngx_destroy_pool()
{
    ngx_pool_large_s *l;
    ngx_pool_cleanup_s *clean;
    for (clean = pool->cleanup; clean; clean = clean->next) 
    {
        clean->handler(clean->data);
        std::cout << "free clean :" << clean->data << std::endl;
    }
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            std::cout << "free large :" << l->alloc << std::endl;
            ngx_pfree(l->alloc);
        }
    }
    for (ngx_pool_s *p = pool->current; p != NULL;) {
        ngx_pool_s *next = p->d.next;
        std::cout << "free pool :" << p << std::endl;
        ngx_pfree(p);
        p = next;
    }
}
void  ngx_mem_pool::ngx_reset_pool()
{
    ngx_pool_s *p;

    for (auto *l = pool->large; l; l = l->next) {
        if (l->alloc) {
            std::cout << "free large :" << l->alloc << std::endl;
            ngx_pfree(l->alloc);
        }
    }

    for (p = pool->current; p != NULL;) {
        std::cout << "reset pool :" << p << std::endl;
        p->d.last = (u_char*)p + sizeof(ngx_pool_data_t);
        p->d.failed = 0;
        p = p->d.next;
    }

    pool->d.last = (u_char*)pool + sizeof(ngx_pool_s);
    pool->large = nullptr;
}
void  ngx_mem_pool::ngx_pfree(void *p)
{
    free(p);
}
void  ngx_mem_pool::ngx_cleanup_add(ngx_pool_cleanup_pt handler, void *data)
{
    ngx_pool_cleanup_s *c = (ngx_pool_cleanup_s*)ngx_palloc_small(sizeof(ngx_pool_cleanup_s), 1);
    if (!c) return;
    std::cout << "add clean :" << c << std::endl;
    c->handler = handler;
    c->data = data;
    c->next = pool->cleanup;
    pool->cleanup = c;
}
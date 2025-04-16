#include <iostream>
#include <cassert>
#include "ngx_mem_pool.h"

// 清理回调示例：模拟资源释放
void cleanup_handler(void* data) {
    std::cout << "Cleaning up: " << data << std::endl;
}

void test_ngx_mem_pool() {
    // 测试1: 创建内存池
    ngx_mem_pool pool;
    assert(pool.ngx_create_pool(4096) == true);
    std::cout << "Test 1: Pool created successfully\n";

    // 测试2: 小块内存分配与对齐
    void* small1 = pool.ngx_palloc(128);
    assert(small1 != nullptr);
    void* small2 = pool.ngx_palloc(256);
    assert(small2 != nullptr);
    std::cout << "Test 2: Small allocations OK\n";

    // 测试3: 大块内存分配
    void* large1 = pool.ngx_palloc(8192);
    assert(large1 != nullptr);
    std::cout << "Test 3: Large allocation OK\n";

    // 测试4: 内存对齐验证
    uintptr_t addr = reinterpret_cast<uintptr_t>(small1);
    assert((addr & (NGX_ALIGNMENT - 1)) == 0 && "Unaligned memory!");
    std::cout << "Test 4: Alignment verified\n";

    // 测试5: 清零分配
    int* zeroed = static_cast<int*>(pool.ngx_pcalloc(100 * sizeof(int)));
    for (int i = 0; i < 100; ++i) {
        assert(zeroed[i] == 0 && "Memory not zeroed!");
    }
    std::cout << "Test 5: pcalloc zero-initialized\n";

    // 测试6: 清理回调
    int dummy_data = 42;
    pool.ngx_cleanup_add(cleanup_handler, &dummy_data);
    std::cout << "Test 6: Cleanup callback registered\n";

    // 测试7: 内存池重置
    pool.ngx_reset_pool();
    void* post_reset = pool.ngx_palloc(512);
    assert(post_reset != nullptr);
    std::cout << "Test 7: Pool reset successful\n";

    // 测试8: 内存池销毁（自动触发清理回调）
    pool.ngx_destroy_pool();
    std::cout << "Test 8: Pool destroyed (check cleanup output)\n";

    // 压力测试
    ngx_mem_pool stress_pool;
    stress_pool.ngx_create_pool(1024 * 1024); // 1MB pool
    for (int i = 0; i < 1000; ++i) {
        void* block = stress_pool.ngx_palloc(rand() % 2048 + 1);
        assert(block != nullptr);
    }
    stress_pool.ngx_destroy_pool();
    std::cout << "Stress test: 1000 random allocations OK\n";
}

int main() {
    test_ngx_mem_pool();
    std::cout << "All tests passed!\n";
    return 0;
}
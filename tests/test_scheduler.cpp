#include "harness.hpp"

#include "habitat/scheduler.h"

static int g_a = 0;
static int g_b = 0;

static void task_a(uint32_t, void*) {
    ++g_a;
}
static void task_b(uint32_t, void*) {
    ++g_b;
}

void test_scheduler() {
    g_a = 0;
    g_b = 0;
    habitat::Scheduler sch;
    EXPECT_TRUE(sch.add(task_a, 100, nullptr, true));
    EXPECT_TRUE(sch.add(task_b, 250, nullptr, false));
    EXPECT_TRUE(!sch.add(nullptr, 10));
    EXPECT_TRUE(!sch.add(task_a, 0));

    sch.tick(0);
    EXPECT_EQ(g_a, 1); // run_immediately
    EXPECT_EQ(g_b, 0); // armed, first fire at 250

    sch.tick(99);
    EXPECT_EQ(g_a, 1);
    sch.tick(100);
    EXPECT_EQ(g_a, 2);
    sch.tick(250);
    EXPECT_EQ(g_b, 1);
    sch.tick(500);
    EXPECT_EQ(g_b, 2);

    // Wrap-safe: now=10, next=0xFFFFFFF0 would be treated as overdue if we used signed subtract
    // incorrectly the other way. Here next is in the future after 500+250.
}

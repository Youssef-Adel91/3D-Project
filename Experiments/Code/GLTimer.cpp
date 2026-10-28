#include "GLTimer.h"
#include <cassert>
#include <stdexcept>

GLTimerQuery::GLTimerQuery(int ringSize) : ringSize_(ringSize) {
    if (ringSize_ < 4) ringSize_ = 4;
    queries_.resize(ringSize_);
    glGenQueries(ringSize_, queries_.data());
    // Pre-populate the free pool
    for (int i = 0; i < ringSize_; ++i) {
        pending_.push_back(0);  // sentinel — not yet issued
    }
    pending_.clear();
}

GLTimerQuery::~GLTimerQuery() {
    if (!queries_.empty()) {
        glDeleteQueries((GLsizei)queries_.size(), queries_.data());
    }
}

void GLTimerQuery::Begin() {
    assert(!inQuery_ && "Nested GL timer queries are not allowed");
    // If all ring slots are occupied by pending queries, stall for the oldest.
    if ((int)pending_.size() >= ringSize_) {
        GLuint q = pending_.front();
        GLuint result = 0;
        GLint available = 0;
        // Blocking wait for oldest
        while (!available) {
            glGetQueryObjectiv(q, GL_QUERY_RESULT_AVAILABLE, &available);
        }
        glGetQueryObjectuiv(q, GL_QUERY_RESULT, &result);
        results_.push_back(result * 1e-6);  // ns -> ms
        pending_.erase(pending_.begin());
        // Return q to the free list (re-use via static pool)
        // We reuse by index cycling — currentQuery_ tracks which to use next.
    }

    DrainCompleted();

    // Pick a query object from the ring.
    // We use a rotating index based on how many we've issued total.
    static int issued = 0;
    currentQuery_ = queries_[issued % ringSize_];
    issued++;

    glBeginQuery(GL_TIME_ELAPSED, currentQuery_);
    inQuery_ = true;
}

void GLTimerQuery::End() {
    assert(inQuery_ && "GLTimerQuery::End() called without Begin()");
    glEndQuery(GL_TIME_ELAPSED);
    inQuery_ = false;
    pending_.push_back(currentQuery_);
}

void GLTimerQuery::DrainCompleted() {
    // Non-blocking: check each pending query from oldest to newest.
    while (!pending_.empty()) {
        GLuint q = pending_.front();
        GLint available = 0;
        glGetQueryObjectiv(q, GL_QUERY_RESULT_AVAILABLE, &available);
        if (!available) break;
        GLuint64 result = 0;
        glGetQueryObjectui64v(q, GL_QUERY_RESULT, &result);
        results_.push_back(result * 1e-6);  // ns -> ms
        pending_.erase(pending_.begin());
    }
}

bool GLTimerQuery::Pop(double& outMs) {
    DrainCompleted();
    if (results_.empty()) return false;
    outMs = results_.front();
    results_.erase(results_.begin());
    return true;
}

void GLTimerQuery::Flush() {
    // Blocking drain of all pending queries.
    while (!pending_.empty()) {
        GLuint q = pending_.front();
        GLuint64 result = 0;
        glGetQueryObjectui64v(q, GL_QUERY_RESULT, &result);  // blocks until ready
        results_.push_back(result * 1e-6);
        pending_.erase(pending_.begin());
    }
}

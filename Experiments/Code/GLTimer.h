#pragma once
#ifndef _H_GLTIMER_
#define _H_GLTIMER_

#include "glad.h"
#include <vector>
#include <algorithm>

// GLTimerQuery: ring buffer of GL_TIME_ELAPSED queries.
// Rules enforced:
//   - Ring size >= 4 so we never read a freshly-issued query.
//   - Begin()/End() issue a query for the current frame.
//   - Collect() retrieves completed results (FIFO order, with readback lag).
//   - Never nest queries.
// 
// Usage:
//   GLTimerQuery timer(4);
//   for (int frame = 0; frame < N; ++frame) {
//       timer.Begin();
//       DrawStuff();
//       timer.End();
//       // Collect completed samples (available after ~4 frames lag)
//       double gpuMs;
//       while (timer.Pop(gpuMs)) results.push_back(gpuMs);
//   }
//   // Drain remaining queries after the loop:
//   timer.Flush();
//   while (timer.Pop(gpuMs)) results.push_back(gpuMs);

class GLTimerQuery {
public:
    explicit GLTimerQuery(int ringSize = 8);
    ~GLTimerQuery();

    // Issue Begin+End around a draw call.
    void Begin();
    void End();

    // Non-blocking pop: returns true and sets ms if a result is available.
    bool Pop(double& outMs);

    // Blocking drain: wait for all pending queries to complete.
    void Flush();

    // Number of results ready to pop.
    int Available() const { return (int)results_.size(); }

private:
    int              ringSize_;
    std::vector<GLuint> queries_;
    std::vector<GLuint> pending_;  // FIFO of issued queries
    std::vector<double> results_;  // completed results

    bool inQuery_ = false;
    GLuint currentQuery_ = 0;

    // Drain any completed (non-blocking)
    void DrainCompleted();
};

#endif

---
layout: chapter
title: Introduction
utterance_id: introduction_draft
---

## About

Linuxviewer is a Vulkan Engine, specifically designed for
the development of 3D graphical Linux applications utilizing
the [Vulkan](https://www.khronos.org/vulkan/) graphics and compute API.

Vulkan is a new API by the [Khronos group](https://www.khronos.org/) (known for OpenGL)
that is designed to be closer to the underlying hardware of modern graphics cards.
Vulkan also allows applications to spread rendering workload across multiple CPU threads,
resulting in an additional benefit in terms of speed for complex applications.

Nonetheless, although Vulkan offers greater control and performance, it comes with a
steeper learning curve compared to OpenGL. This may make it harder to begin working with
and results in longer development times, particularly for smaller projects or prototypes.

<img src="{{ '/assets/VulkanMemeBigBook.png' | relative_url }}" alt="Drawing a triangle with Vulkan - Meme" id="bigbook" />

Linuxviewer is build on top of the `aicxx` git submodules, roughly developed between 2010 and 2021.
These submodules provide building blocks for C++ applications, focusing on speed, robustness,
and efficient concurrency management to harness the full potential of multicore processors.

Besides building on top of more than 10 years of development that went into `aicxx`,
for things like multi-threading, I/O, locking, debug support, memory management and
so on, the aim of this Vulkan Engine project is to make it much easier to use Vulkan.

The motto of `aicxx` is "if it compiles, it works". Linuxviewer tries to up hold that promise
even for Vulkan applications.

## Linux only

The main reason linuxviewer is linux-only is because I have decades of experience with
Linux and zero with Windows (or Mac). I don't even own a Windows box, so I wouldn't be able
to test anything.

However, once having submitted to not be portable, this opened the door to writing faster code,
specifically tuned for linux. Note that mainly, if not only, it's the `aicxx` modules that are linux-specific.
Most important being the use of `epoll` for socket I/O and `futex` for the thread pool.
But there are other optimizations like "guessing" how `malloc` allocates memory (to avoid extra overhead)
specifics of mutex timings, costs of system calls, and the linux scheduler.
None of those things would be in the way of a straight-forward porting to another operating system,
but might not give the fine-tuning anymore that was aimed for.

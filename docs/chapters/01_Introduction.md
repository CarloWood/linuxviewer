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

<img src="{{ '/assets/VulkanMemeBigBook.png' | relative_url }}" alt="Drawing a triangle with Vulkan - Meme" id="img_bigbook" />

Linuxviewer is build on top of the `aicxx` git submodules, roughly developed between 2010 and 2021.
These submodules provide building blocks for C++ applications, focusing on speed, robustness,
and efficient concurrency management to harness the full potential of multicore processors.

Besides building on top of more than 10 years of development that went into `aicxx`,
for things like multi-threading, I/O, locking, debug support, memory management and
so on, the aim of linuxviewer is to make it much easier to use Vulkan.
The motto of `aicxx` is "if it compiles, it works".
Linuxviewer tries to up hold that promise even for Vulkan applications.

## Linux only

The primary reason Linuxviewer is Linux-only is that I have decades of experience with Linux
and none with Windows (or Mac). I don't even own a Windows machine, so I wouldn't be able to test anything.

However, after committing to a non-portable design, this opened the door to writing faster code specifically optimized for Linux.
It's worth noting that mainly, if not exclusively, the `aicxx` modules are Linux-specific.
The most significant aspects include the use of `epoll` for I/O and `futex` thread pool scheduling.
Additionally, there are other optimizations such as "guessing" how malloc allocates memory (to avoid extra overhead),
mutex timing specifics, system call costs, and the Linux scheduler.
None of these factors would impede a straightforward port to another operating system,
but they might not provide the fine-tuning that was originally aimed for.

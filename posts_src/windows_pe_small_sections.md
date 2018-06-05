
---
title:  Windows executables and section alignment
date:   June, 2018
author: Morten H. Solvang
---

In general, in an executable, the sections (`.text`, `.data`, etc.) are expected to be loaded into separate memory pages. In fact, this is relied upon for features such as setting different read/write/execute permissions for the different sections in an executable, as these properties are set on a per-memory-page basis. When generating an executable file, you (or the compiler/linker) specifies a section alignment, which in general must be greater than or equal to the page size.

On windows, you can however set the section alignment to be lower than the page size (The relevant field in a windows executable is `SectionAlignment`). This has multiple effects, some of which are unfortunately not directly documented.  

For one, it forces the executable files sections to be loaded directly into memory, without sections being moved around. It is worth noting that this means `SectionAlignment` and `FileAlignment` must be equal.  

It also disables unique permissions per section. For example, a write-protected section will actually be writable. I am not how permissions for the section are actually decided, though it seems like all sections get full permissions in this case. There is unfortunately no mention of this on MSDN...  

Lastly, address space layout randomization gets disabled, meaning any RVAs in the executable can be used as absolute addresses.  

I assume the reason for this behavior is for backwards compatibility, though that is pure conjecture.
For reference, see the [MSDN documentation](http://www.microsoft.com/whdc/system/platform/firmware/PECOFF.mspx) for the PE-format.

_Note: I'm not sure how the situation is on linux/osx, though from what I gather this behaviour is not present there_

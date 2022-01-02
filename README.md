# MarFS

> This is a sub-project of Melvix and shouldn't be used otherwise. It's not fast and 32-bit only.

As of right now, there are two utility programs for GNU/Linux in this project:
* `mkfs` creates an empty MarFS filesystem on a specified image
* `fuse` mounts a specified MarFS-formatted image onto a specified directory

`mkfs` as well as `fuse` are quick and dirty solutions without performance in mind. You may find better implementations in the main Melvix repository.

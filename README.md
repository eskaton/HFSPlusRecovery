# HFSPlusRecovery

A tool to recover HFS+ partitions from disk images.

## History

In 2008 I had a disk crash which rendered my home directory unusable. The first thing I did was creating an image using `dd`. Unfortunatelly, I wasn't able to find a recovery software that worked with images and I didn't want to access the disk again because I feared it would damage it further. So I wrote this tool with which I was able to recover most of my data.

## How does it work?

It works bottom up from the files and tries to recreate the directory hierarchy. If it is unable to build the whole path of a file, the file is recovered in a lost+found directory.

## Usage

HFSPlusRecovery takes two parameters:

* A device which can be a raw device partition or an image thereof

* The path where to recover the disk

HFSPlusRecovery can also operate on a whole disk, but in this case expects a third parameter with the offset to the start of a partition.

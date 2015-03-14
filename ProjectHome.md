**I've kinda given up on this because i have work to do.**

Find the more current project page on http://swehack.se/project/bicebot

It's easy to use and you can write your own modules for it in C.

Because it is written in C without any external third party libraries i cut out a lot of middlemen and make it faster. Also, more vulnerable, but maybe we can fix that with the help of the community.

I use -ldl functions to load and unload modules into it's address space, it's all very simple and didn't take long to write.

You can send a kill -USR1 signal to the bot and make it reload all it's modules without dropping it's connection.

It's all developed on and for FreeBSD and I write ANSI C code which should conform to POSIX standards.

Some modules don't conform to XOPEN=600 because they use ndbm or other functions that are not standardized properly.

**Observe** that this is only a project i have fun with in my free time as a hobby. I don't take it seriously but as Google are kind enough to host it for me i'll put it out there and see if anyone likes it.
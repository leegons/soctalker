# soctalker
一个 C/S 模式的socket聊天工具

# usage

**server**:
``` shell
$ ./server 9285
 SocTalker 0.1 started.
 Listen on 9285.
> hello
From Admin: hello
> Codename E connected (127.0.0.1:35764)
From Admin: Codename E connected.
From E: hello admin
Codename F connected (127.0.0.1:35766)
From Admin: Codename F connected.
From F: hello, I'm in.

> hi, all members.
From Admin: hi, all members.
> From Admin: Codename E disconnected.
```

**client 1**
``` shell
$ nc 127.0.0.1 9285
 SocTalker 0.1
 Use (127.0.0.1:35764)
From Admin: Welcome, Codename E.
hello admin
From Admin: Codename F connected.
From F: hello, I'm in.
From Admin: hi, all members.
^C
$
```

**client 2**
``` shell
$ nc 127.0.0.1 9285
 SocTalker 0.1
 Use (127.0.0.1:35766)
From Admin: Welcome, Codename F.
hello, I'm in.
From Admin: hi, all members.
From Admin: Codename E disconnected.
```

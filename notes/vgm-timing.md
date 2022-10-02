[vgmrips forum - VGM file format timing issue](https://vgmrips.net/forum/viewtopic.php?p=12162#p12162)

Reply from ValleyBell
>In general, VGM timing assumed that the action of "sending a command" takes 0 time.
>Only "wait" commands cause the time to advance. You need to be aware of that concept.
>
>Thus, when sending a command takes some real time, you need to subtract that time from the next delay.
>When I write timing code, I usually have a timestamp for "time of the next command", which is increased whenever there is a "wait" command.
>And when I'm sleeping, I always wait until that timestamp is reached. (If the real time is already past it, then of course there is no wait.)

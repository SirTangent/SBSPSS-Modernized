![Supercharged Logo](docs/assets/supercharged-logo.png)

# Spongebob Squarepants: SuperSponge [Supercharged]

The original PS1 game ported over to modern operating systems (Windows 11)

## What the heck is this project?

Back in 2001, SuperSponge was released for the original PlayStation (PS1). It was one of the first video games to be released under the franchise, followed by many more successful titles.

Many years later, the full source code by Climax Development was released on the internet with everything needed to compile the game. The main hurdle was the development environment required, as compilation could only be done on a Windows 98 VM. The other elephant in the room was the target platform. Being solely a PS1 project, the code was written to work with it's hardware using the PSYQ SDK. In other words, the codebase would require major re-work with a new abstraction layer to operate on a modern OS (and system architecture)

This project substitutes the PSYQ SDK and its toolchain with a modern one, targeting modern hardware. It's similar to PsyCross, but with some deliberations. The project lives in the `port` directory with the build scripts needed to generate a Windows executable. The game itself behaves and acts like the real thing from the PlayStation. There is no emulation happening in the backend, just  the game running on today's hardware for today's operating system. Hope you enjoy my project!

## How to use?
TODO: Work in progress...

## Frequently Asked Questions (Probably...)

### Can't you just emulate the game anyway?
Yes, there is nothing really stopping you from compiling for the PS1 and emulating it. However, I believe there are benefits from stripping away the translation layer and applying optimizations that come from modern platforms. It also extends the game to run on less powerful (not that a PS1 game is power hungry) hardware.

### Why not use PsyCross?
I guess there isn't a reason not to, but personally, I wanted to see what it took to port a game over from scratch with minimal dependencies. I'm aware of it's capabilities, and it's success with porting over titles such as Driver 2. My long-term goal is to re-design some of the mechanisms that will diverge from the translation layer completely.

### How dare you use AI for coding slop?
If it's not obvious, the project used generative AI and agentic coding for a large chunk of the work. I do acknowlege that similar projects want to avoid AI for good reasons, which I can respect.

Realisticaly, these projects have large scopes as a result of how coupled the game logic is with PSYQ. From what I read, it took almost two years to port Driver 2 using Psy-Cross, which I understood had to be reverse engineered. From the get-go, porting a game can require an extensive overhaul of the codebase and therefore a lot of man hours.

Given this was a solo project and that I have a full-time software engineering job (yes, we use AI there too), using it responsibly really cut down on the amount of time needed to get to an MVP. I still had input on some of the high-level design decisions and try to ensure the software has adequate testing. I do acknowlege there are inherit risks with quality and technical-debt build-up. At the end of the day, you still need to posses some understanding of the technical details and steer these models correctly.

> "If you can’t beat them, join them”

### Plan to support other platforms?

Yes, Definately! Right now, the MVP is to get it ported for Windows 11. In addition, it only compiles as a 32-bit application (Using WoW64) and have it in-scope to refactor it to target x86-64. Once I finish the Win11 milestone, I can start to work on other ports. Here are some on the to-do list.

* MacOS
* Linux
* Android
* WebAssembly

btw, yes I could have used a cross-platform framework.
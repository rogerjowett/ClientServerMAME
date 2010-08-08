Alright, the way it should work if you follow these steps:

1) Find one or more friends (this can often be the hardest part)
2) Both of you get the rom you are going to play and ClientServerMAME from here: 

http://www.underworldhockeyclub.com/ClientServerMAME/ClientServerMAME_v0_1_Win32.zip

3) If anyone is running linux, he or she will need some additional packages: SDL and expat.  Find them and install them from your package manager.
4) Decide who is going to be the server
5) The person who is the server needs to make sure that TCP port 5805 is open and points to their IP. This will probably require port forwarding or something like that.  Most games have this issue, so you can find info on it on the internet if you need help.
6) The server needs to tell everyone else his IP address or hostname
7) Have the server start the game like this:
mamesdl.exe <rom> -server
8) Have the clients connect like this (make sure you fill in (rom) and (hostname) with real values):
mamesdl.exe (rom) -client -hostname (hostname)
9) Everyone uses player 1 controls on their own machine.  ClientServerMAME currently does not support having more than one player at one computer, and all other player inputs are ignored.
10) There isn't much in the way of error handling at this point, so don't do anything stupid like try to run the wrong rom or try to load a rom while you are in the game or it will crash hard.


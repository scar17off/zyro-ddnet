#ifndef GAME_CLIENT_COMPONENTS_CHEAT_GUI_H
#define GAME_CLIENT_COMPONENTS_CHEAT_GUI_H

#include <game/client/component.h>

//using asio::ip::tcp;

/*
    NOTE: THIS IS NOT THE GUI, THIS IS THE SERVER THAT THE GUI CONNECTS TO. (js gui? huh???) (yeah it's a bit of a mess)
    THIS MAY BE DEPRECATED IN THE FUTURE BECAUSE IT'S NOT THE BEST WAY TO DO IT.
    first i was trying to implement the gui using CEF (Chromium Embedded Framework) but it was a pain in the ass to setup the browser.

    This class reprecents the GUI's TCP server component.
    The actual GUI is implemented as an Electron overlay application which communicates with the game client via a TCP connection.
*/
class CGui : public CComponent 
{
public:
	virtual int Sizeof() const override { return sizeof(*this); }

	void RunServer();

private:
};

#endif
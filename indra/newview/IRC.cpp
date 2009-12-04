/*
	cpIRC - C++ class based IRC protocol wrapper
	Copyright (C) 2003 Iain Sheppard

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	Contacting the author:
	~~~~~~~~~~~~~~~~~~~~~~

	email:	iainsheppard@yahoo.co.uk
	IRC:	#magpie @ irc.quakenet.org
*/

/*

	Modified for Winsock support by zstars. Sorry, the code is crappy, but it works.

*/
#include "llviewerprecompiledheaders.h"
#include "IRC.h"
#include "llmd5.h"

#include <sstream>

#ifdef LL_WINDOWS
#include <windows.h>
#else
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#define closesocket(s) close(s)
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#endif

using namespace std;

#if LL_WINDOWS
#else
#define sprintf_s snprintf
#endif

IRC::IRC()
{
	hooks=0;
	chan_users=0;
	connected=false;
	sentnick=false;
	sentpass=false;
	sentuser=false;
	allparticipants.clear();
	allcorespondingNick.clear();
	cur_nick=0;
}

IRC::~IRC()
{
	if (hooks)
		delete_irc_command_hook(hooks);
}

void IRC::insert_irc_command_hook(irc_command_hook* hook, const char* cmd_name, int (*function_ptr)(char*, irc_reply_data*, void*))
{
	if (hook->function)
	{
		if (!hook->next)
		{
			hook->next=new irc_command_hook;
			hook->next->function=0;
			hook->next->irc_command=0;
			hook->next->next=0;
		}
		insert_irc_command_hook(hook->next, cmd_name, function_ptr);
	}
	else
	{
		hook->function=function_ptr;
		hook->irc_command=new char[strlen(cmd_name)+1];
		strcpy(hook->irc_command, cmd_name);
	}
}

void IRC::hook_irc_command(const char* cmd_name, int (*function_ptr)(char*, irc_reply_data*, void*))
{
	if (!hooks)
	{
		hooks=new irc_command_hook;
		hooks->function=0;
		hooks->irc_command=0;
		hooks->next=0;
		insert_irc_command_hook(hooks, cmd_name, function_ptr);
	}
	else
	{
		insert_irc_command_hook(hooks, cmd_name, function_ptr);
	}
}

void IRC::delete_irc_command_hook(irc_command_hook* cmd_hook)
{
	if (cmd_hook->next)
		delete_irc_command_hook(cmd_hook->next);
	if (cmd_hook->irc_command)
		delete cmd_hook->irc_command;
	delete cmd_hook;
}

int IRC::start(char* server, int port,char* nick,char* user, char* name, char* pass)
{

	#ifdef LL_WINDOWS
	HOSTENT* resolv;
	#else
	hostent* resolv;
	#endif
	sockaddr_in rem;
	
	if (connected)
		return 1;
#if LL_WINDOWS
	
	WSADATA             wsa;
	memset(&wsa, 0x00, sizeof(WSADATA));
    if(WSAStartup(MAKEWORD(2, 0), &wsa) != 0x0)
    {
        warn("Socket Initialization Error. Program aborted\n");
		return 1;
    }

	irc_socket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (irc_socket==INVALID_SOCKET)
	{
		warn("Invalid sockets");
		return 1;
	}
#else
	irc_socket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (irc_socket==-1)
	{
		warn("Invalid sockets");
		return 1;
	}
#endif
	resolv=gethostbyname(server);
	/*
		gets(in);
		IPHostEntry* hostInfo = Dns::GetHostByName(in);
		printf(hostInfo);
		*/
	llinfos<<llformat("Host Name(%s) Resolved To: %p \n",server,resolv)<<llendl;
	if (!resolv)
	{
#ifdef LL_WINDOWS
		closesocket(irc_socket);
#else
		close(irc_socket);
#endif
		return 1;
	}
	memcpy(&rem.sin_addr, resolv->h_addr, 4);
	rem.sin_family=AF_INET;
	rem.sin_port=htons(port);
	/*
Global $sock = TCPConnect(TCPNameToIP($host), $port)
If @error Then
	_Log("We have failed to connect to server " & $host & " at port " & $port)
	MsgBox(1, $Name & $Ver & " Error", "We cannot connect to the IRC Server." & @CRLF & "Host : " & $host & @CRLF & "Port : " & $port)
	Exit
EndIf
Sleep(1000)
_sendmsg("NICK " & $nick)
Sleep(1000)
_sendmsg('USER ' & $name & ' "' & $mailhost & '" "' & $host & '" :' & $name)
Sleep(1000)
_sendmsg("USERHOST " & $nick)


	*/

	if (connect(irc_socket, (const sockaddr*)&rem, sizeof(rem))==SOCKET_ERROR)
	{
		#ifdef LL_WINDOWS
		llinfos<<llformat("Failed to connect: %d\n", WSAGetLastError())<<llendl;
		#endif
		closesocket(irc_socket);

		warn("Failed to connect wsaerror");
		return 1;
	}

	/*dataout=_fdopen(irc_socket, "w");*/
	//datain=fdopen(irc_socket, "r");
	
	//if (!dataout /*|| !datain*/)
	//{
	//	printf("Failed to open streams!\n");
	//	closesocket(irc_socket);
	//	return 1;
	//}
	
	connected=true;
	
	cur_nick=new char[strlen(nick)+1];
	strcpy(cur_nick, nick);

	string sout = "";
	sout += "PASS "; sout += pass; sout += "\r\n";
	sout += "NICK "; sout += nick; sout += "\r\n";
	sout += "USER "; sout += user; sout += " * 0 :"; sout += name; sout += "\r\n";
	int ret = send(irc_socket, sout.c_str(), (int)sout.size(), 0);

	//printf("IP: %s",irc_socket
	if(ret == -1)
	{
		warn("send failed");
		return 1;
	}

	/*fprintf(dataout, "PASS %s\r\n", pass);
	fprintf(dataout, "NICK %s\r\n", nick);
	fprintf(dataout, "USER %s * 0 :%s\r\n", user, name);
	
	fflush(dataout);		*/

	return 0;
}

void IRC::disconnect()
{
	if (connected)
	{
		//fclose(dataout);
		llinfos<<"Disconnected from server.\n"<<llendl;
		connected=false;
		quit("Leaving");
		#ifdef LL_WINDOWS
		shutdown(irc_socket, 2);
		#endif
		closesocket(irc_socket);
	}
}

int IRC::quit(const char* quit_message)
{
	if (connected)
	{
		if (quit_message)
		{
				string sout;
				sout = "QUIT "; sout += quit_message; sout += "\r\n";
				int ret = send(irc_socket, sout.c_str(), (int)sout.size(), 0);
				if(ret == -1) return 1;
		}
			//fprintf(dataout, "QUIT %s\r\n", quit_message);
		else
		{
				string sout;
				sout = "QUIT "; sout += "\r\n";
				int ret = send(irc_socket, sout.c_str(), (int)sout.size(), 0);
				if(ret == -1) return 1;
		}
			//fprintf(dataout, "QUIT\r\n");
		//if (fflush(dataout))
		//	return 1;
	}
	return 0;
}

int IRC::message_loop()
{
	char buffer[1024];
	int ret_len;

	if (!connected)
	{
		warn("Not connected!\n");
		return 1;
	}

	while (1)
	{
		ret_len=recv(irc_socket, buffer, 1023, 0);
		if (ret_len==SOCKET_ERROR || !ret_len)
		{
			return 1;
		}
		buffer[ret_len]='\0';
		split_to_replies(buffer);
	}

	return 0;
}
void IRC::warn(std::string warning)
{
	
		llwarns << warning << llendl;
}

void IRC::split_to_replies(char* data)
{
	char* p;

	while ((p=strstr(data, "\r\n")))
	{
		*p='\0';
		parse_irc_reply(data);
		data=p+2;
	}
}

int IRC::is_op(char* channel, char* nick)
{
	channel_user* cup;

	cup=chan_users;
	
	while (cup)
	{
		if (!strcmp(cup->channel, channel) && !strcmp(cup->nick, nick))
		{
			return cup->flags&IRC_USER_OP;
		}
		cup=cup->next;
	}

	return 0;
}
LLSD IRC::getSpeakersLLSD()
{
	
	channel_user* cup;

	cup=chan_users;
	int i=0;
	
	LLSD speakers;
	while (cup)
	{
		LLSD speaker;
		std::string strnick = std::string(cup->nick);
		LLUUID uid;
		//llinfos << "Generating uuid from " << strnick << " and " << std::string(cup->channel) << llendl;
		uid.generate(strnick+"lgg"+std::string(cup->channel));

		vector<LLUUID>::iterator result;
		result = find( allparticipants.begin(), allparticipants.end(),uid );
		if( result == allparticipants.end() ) 
		{
			llinfos << "Generating uuid from |" << strnick << "| and |" << std::string(cup->channel) << "| it was " << uid.asString() << llendl;

			allparticipants.push_back(uid);
			allcorespondingNick.push_back(strnick);
		}
		if(strnick != "")
		{
			//std::string toMakeUUID;
			//LLMD5 hashedName;
			//LLMemoryStream str((U8*)strnick.c_str(), strnick.length());
			//hashedName.update(str);
			//hashedName.finalize();
			//hashedName.hex_digest(toMakeUUID);
			//LLUUID hashedUUID(toMakeUUID);
			
			speaker["irc_agent_id"]=uid;
			speaker["irc_agent_name"]=strnick;
			speaker["irc_channel"]=std::string(cup->channel);
			speaker["irc_mode"]=std::string("nvm");
			speaker["irc_agent_mod"]=((cup->flags&IRC_USER_OP)|(cup->flags&IRC_USER_HALFOP));
			speakers[i]=speaker;
			i++;
		}
		cup=cup->next;
	}
	return speakers;
}
int IRC::is_voice(char* channel, char* nick)
{
	channel_user* cup;

	cup=chan_users;
	
	while (cup)
	{
		if (!strcmp(cup->channel, channel) && !strcmp(cup->nick, nick))
		{
			return cup->flags&IRC_USER_VOICE;
		}
		cup=cup->next;
	}

	return 0;
}

void IRC::parse_irc_reply(char* data)
{
	char* hostd;
	char* cmd;
	char* params;
//	char buffer[514];
	irc_reply_data hostd_tmp;
	channel_user* cup;
	char* p;
	char* chan_temp;

	hostd_tmp.target=0;

	//printf("%s\n", data);
	//llinfos << "data is->" << data << llendl;
	if (data[0]==':')
	{
		hostd=&data[1];
		
		//llinfos << "hostd is->" << hostd << llendl;
		cmd=strchr(hostd, ' ');
		if (!cmd)
			return;
		*cmd='\0';
		cmd++;
		params=strchr(cmd, ' ');
		if (params)
		{
			*params='\0';
			params++;
		}

		hostd_tmp.nick=hostd;
		
		//llinfos << "hostdtempnice->" << hostd_tmp.nick << llendl;
		hostd_tmp.ident=strchr(hostd, '!');
		if (hostd_tmp.ident)
		{
			*hostd_tmp.ident='\0';
			hostd_tmp.ident++;
			hostd_tmp.host=strchr(hostd_tmp.ident, '@');
			if (hostd_tmp.host)
			{
				*hostd_tmp.host='\0';
				hostd_tmp.host++;
			}
		}

		if (!strcmp(cmd, "JOIN"))
		{
			cup=chan_users;
			if (cup)
			{
				while (cup->nick)
				{
					if (!cup->next)
					{
						cup->next=new channel_user;
						cup->next->channel=0;
						cup->next->flags=0;
						cup->next->next=0;
						cup->next->nick=0;
					}
					cup=cup->next;
				}
				if(strstr(params," :"))params+=2;
				if(strstr(params,":"))params+=1;

				cup->channel=new char[strlen(params)+1];
				strcpy(cup->channel, params);
				cup->nick=new char[strlen(hostd_tmp.nick)+1];
				strcpy(cup->nick, hostd_tmp.nick);
			}
		}
		else if (!strcmp(cmd, "PART"))
		{
			channel_user* d;
			channel_user* prev;

			d=0;
			prev=0;
			cup=chan_users;
			while (cup)
			{
				if (!strcmp(cup->channel, params) && !strcmp(cup->nick, hostd_tmp.nick))
				{
					d=cup;
					break;
				}
				else
				{
					prev=cup;
				}
				cup=cup->next;
			}
			if (d)
			{
				if (d==chan_users)
				{
					chan_users=d->next;
					if (d->channel)
						delete [] d->channel;
					if (d->nick)
						delete [] d->nick;
					delete d;
				}
				else
				{
					if (prev)
					{
						prev->next=d->next;
					}
					chan_users=d->next;
					if (d->channel)
						delete [] d->channel;
					if (d->nick)
						delete [] d->nick;
					delete d;
				}
			}
		}
		else if (!strcmp(cmd, "QUIT"))
		{
			channel_user* d;
			channel_user* prev;

			d=0;
			prev=0;
			cup=chan_users;
			while (cup)
			{
				if (!strcmp(cup->nick, hostd_tmp.nick))
				{
					d=cup;
					if (d==chan_users)
					{
						chan_users=d->next;
						if (d->channel)
							delete [] d->channel;
						if (d->nick)
							delete [] d->nick;
						delete d;
					}
					else
					{
						if (prev)
						{
							prev->next=d->next;
						}
						if (d->channel)
							delete [] d->channel;
						if (d->nick)
							delete [] d->nick;
						delete d;
					}
					break;
				}
				else
				{
					prev=cup;
				}
				cup=cup->next;
			}
		}
		else if (!strcmp(cmd, "KICK"))
		{
			channel_user* d;
			channel_user* prev;

			d=0;
			prev=0;
			cup=chan_users;

			//params is like this #Meta7 Meta7-User354541ac :test
			std::string paramstring(params);
			istringstream iss(paramstring);
			std::string tuser;
			iss >> tuser;//first part we dont need
			iss >> tuser;
			if(!strcmp(tuser.c_str(),cur_nick))
			{
				//we got kicked!!! shut down everything
				chan_users = 0;
			}else
			while (cup)
			{
								  
				if (!strcmp(cup->nick, tuser.c_str()))
				{
					d=cup;
					if (d==chan_users)
					{
						chan_users=d->next;
						if (d->channel)
							delete [] d->channel;
						if (d->nick)
							delete [] d->nick;
						delete d;
					}
					else
					{
						if (prev)
						{
							prev->next=d->next;
						}
						if (d->channel)
							delete [] d->channel;
						if (d->nick)
							delete [] d->nick;
						delete d;
					}
					break;
				}
				else
				{
					prev=cup;
				}
				cup=cup->next;
			}
		}
		else if (!strcmp(cmd, "MODE"))
		{
			char* chan;
			char* changevars;
			channel_user* cup;
			channel_user* d;
			char* tmp;
			int i;
			bool plus;

			chan=params;
			params=strchr(chan, ' ');
			*params='\0';
			params++;
			changevars=params;
			params=strchr(changevars, ' ');
			if (!params)
			{
				return;
			}
			if (chan[0]!='#')
			{
				return;
			}
			*params='\0';
			params++;
		
			plus=false;
			for (i=0; i<(signed)strlen(changevars); i++)
			{
				switch (changevars[i])
				{
				case '+':
					plus=true;
					break;
				case '-':
					plus=false;
					break;
				case 'o':
					tmp=strchr(params, ' ');
					if (tmp)
					{
						*tmp='\0';
						tmp++;
					}
					tmp=params;
					if (plus)
					{
						// user has been opped (chan, params)
						cup=chan_users;
						d=0;
						while (cup)
						{
							if (cup->next && cup->channel)
							{
								if (!strcmp(cup->channel, chan) && !strcmp(cup->nick, tmp))
								{
									d=cup;
									break;
								}
							}
							cup=cup->next;
						}
						if (d)
						{
							d->flags=d->flags|IRC_USER_OP;
						}
					}
					else
					{
						// user has been deopped (chan, params)
						cup=chan_users;
						d=0;
						while (cup)
						{
							if (!strcmp(cup->channel, chan) && !strcmp(cup->nick, tmp))
							{
								d=cup;
								break;
							}
							cup=cup->next;
						}
						if (d)
						{
							d->flags=d->flags^IRC_USER_OP;
						}
					}
					params=tmp;
					break;
				case 'v':
					tmp=strchr(params, ' ');
					if (tmp)
					{
						*tmp='\0';
						tmp++;
					}
					if (plus)
					{
						// user has been voiced
						cup=chan_users;
						d=0;
						while (cup)
						{
							if (!strcmp(cup->channel, params) && !strcmp(cup->nick, hostd_tmp.nick))
							{
								d=cup;
								break;
							}
							cup=cup->next;
						}
						if (d)
						{
							d->flags=d->flags|IRC_USER_VOICE;
						}
					}
					else
					{
						// user has been devoiced
						cup=chan_users;
						d=0;
						while (cup)
						{
							if (!strcmp(cup->channel, params) && !strcmp(cup->nick, hostd_tmp.nick))
							{
								d=cup;
								break;
							}
							cup=cup->next;
						}
						if (d)
						{
							d->flags=d->flags^IRC_USER_VOICE;
						}
					}
					params=tmp;
					break;
				default:
					return;
					break;
				}
				// ------------ END OF MODE ---------------
			}
		}
		else if (!strcmp(cmd, "353"))
		{
			// receiving channel names list
			if (!chan_users)
			{
				chan_users=new channel_user;
				chan_users->next=0;
				chan_users->nick=0;
				chan_users->flags=0;
				chan_users->channel=0;
			}
			cup=chan_users;
			chan_temp=strchr(params, '#');
			if (chan_temp)
			{
				//chan_temp+=3;
				p=strstr(chan_temp, " :");
				if(p)
				{
					*p='\0';
					p+=2;
					while (strchr(p, ' '))
					{
						char* tmp;

						tmp=strchr(p, ' ');
						*tmp='\0';
						tmp++;
						while (cup->nick)
						{
							if (!cup->next)
							{
								cup->next=new channel_user;
								cup->next->channel=0;
								cup->next->flags=0;
								cup->next->next=0;
								cup->next->nick=0;
							}
							cup=cup->next;
						}
						if (p[0]=='@')
						{
							cup->flags=cup->flags|IRC_USER_OP;
							p++;
						}else
						if (p[0]=='%')
						{
							cup->flags=cup->flags|IRC_USER_HALFOP;
							p++;
						}else
						if (p[0]=='&')
						{
							cup->flags=cup->flags|IRC_USER_OP;
							p++;
						}else
						if (p[0]=='~')
						{
							cup->flags=cup->flags|IRC_USER_OP;
							p++;
						}else
						if (p[0]=='!')
						{
							cup->flags=cup->flags|IRC_USER_OP;
							p++;
						}else
						if (p[0]=='+')
						{
							cup->flags=cup->flags|IRC_USER_VOICE;
							p++;
						}
						//llinfos << "chan temp was " << chan_temp << llendl;
						if(strstr(chan_temp," :"))chan_temp+=2;
						if(strstr(chan_temp,":"))chan_temp+=1;
						//llinfos << "chan temp is now " << chan_temp << llendl;
						cup->nick=new char[strlen(p)+1];
						strcpy(cup->nick, p);
						cup->channel=new char[strlen(chan_temp)+1];
						strcpy(cup->channel, chan_temp);
						p=tmp;
					}
					while (cup->nick)
					{
						if (!cup->next)
						{
							cup->next=new channel_user;
							cup->next->channel=0;
							cup->next->flags=0;
							cup->next->next=0;
							cup->next->nick=0;
						}
						cup=cup->next;
					}
					if (p[0]=='@')
					{
						cup->flags=cup->flags|IRC_USER_OP;
						p++;
					}
					else if (p[0]=='+')
					{
						cup->flags=cup->flags|IRC_USER_VOICE;
						p++;
					}
					else if (p[0]=='~')
					{
						//q
						cup->flags=cup->flags|IRC_USER_OP;
						p++;
					}
					else if (p[0]=='1')
					{
						cup->flags=cup->flags|IRC_USER_OP;
						p++;
					}
					else if (p[0]=='&')
					{
						//admin
						cup->flags=cup->flags|IRC_USER_OP;
						p++;
					}//(qaohv)~&@%+

					//llinfos << "chan temp was " << chan_temp << llendl;
					if(strstr(chan_temp," :"))chan_temp+=2;
					if(strstr(chan_temp,":"))chan_temp+=1;
					//llinfos << "chan temp is now " << chan_temp << llendl;
					cup->nick=new char[strlen(p)+1];
					strcpy(cup->nick, p);
					cup->channel=new char[strlen(chan_temp)+1];
					strcpy(cup->channel, chan_temp);
				}
			}
		}
		else if (!strcmp(cmd, "NOTICE"))
		{
			hostd_tmp.target=params;
			params=strchr(hostd_tmp.target, ' ');
			if (params)
				*params='\0';
			params++;
			#ifdef __IRC_DEBUG__
			printf("%s >-%s- %s\n", hostd_tmp.nick, hostd_tmp.target, &params[1]);
			#endif
		}
		else if (!strcmp(cmd, "PRIVMSG"))
		{
			hostd_tmp.target=params;
			params=strchr(hostd_tmp.target, ' ');
			if (!params)
				return;
			*(params++)='\0';
			#ifdef __IRC_DEBUG__
			printf("%s: <%s> %s\n", hostd_tmp.target, hostd_tmp.nick, &params[1]);
			#endif
		}
		else if (!strcmp(cmd, "NICK"))
		{
			
			//llinfos << "hostdtempnice- in nick>" << hostd_tmp.nick << llendl;
			if (!strcmp(hostd_tmp.nick, cur_nick))
			{
				delete [] cur_nick;
				cur_nick=new char[strlen(&params[1])+1];
				strcpy(cur_nick, &params[1]);
			}
			cup=chan_users;
			while(cup)
			{
				
				//llinfos << "loooking at>" << cup->nick << llendl;
				if (!strcmp(hostd_tmp.nick, cup->nick))
				{
					delete cup->nick;
					cup->nick=new char[strlen(hostd_tmp.nick)+1];
					strcpy(cup->nick,&params[1]);
				}
				cup=cup->next;
			}
		}
		/* else if (!strcmp(cmd, ""))
		{
			#ifdef __IRC_DEBUG__
			#endif
		} */
		call_hook(cmd, params, &hostd_tmp);
	}
	else
	{
		cmd=data;
		data=strchr(cmd, ' ');
		if (!data)
			return;
		*data='\0';
		params=data+1;

		if (!strcmp(cmd, "PING"))
		{
			if (!params)
				return;
			string sout;
			sout = "PONG "; sout += &params[1]; sout += "\r\n";
			int ret = send(irc_socket, sout.c_str(), (int)sout.size(), 0);
			if(ret == -1) return;
			//fprintf(dataout, "PONG %s\r\n", &params[1]);
			#ifdef __IRC_DEBUG__
			printf("Ping received, pong sent.\n");
			#endif
			//fflush(dataout);
		}
		else
		{
			hostd_tmp.host=0;
			hostd_tmp.ident=0;
			hostd_tmp.nick=0;
			hostd_tmp.target=0;
			call_hook(cmd, params, &hostd_tmp);
		}
	}
}

void IRC::call_hook(char* irc_command, char* params, irc_reply_data* hostd)
{
	irc_command_hook* p;

	if (!hooks)
		return;

	p=hooks;
	while (p)
	{
		if (!strcmp(p->irc_command, irc_command))
		{
			(*(p->function))(params, hostd, this);
			p=0;
		}
		else
		{
			p=p->next;
		}
	}
}

int IRC::notice(char* target, char* message)
{
	if (!connected)
		return 1;
	string sout;
	sout = "NOTICE "; sout += target; sout += " :"; sout += message; sout += "\r\n";
	int ret = send(irc_socket, sout.c_str(), (int)sout.size(), 0);
	if(ret == -1) return 1;
	//fprintf(dataout, "NOTICE %s :%s\r\n", target, message);
	return 0;
	//return fflush(dataout);
}

int IRC::notice(char* fmt, ...)
{
	va_list argp;
//	char* target;
	
	if (!connected)
		return 1;
	va_start(argp, fmt);

	string sout;
	sout = "NOTICE "; sout += fmt; sout += " :";
	int ret = send(irc_socket, sout.c_str(), (int)sout.size(), 0);
	if(ret == -1) return 1;
	//fprintf(dataout, "NOTICE %s :", fmt);
	char buf[1024];
#if LL_WINDOWS
	vsprintf_s(buf, sizeof(buf), va_arg(argp, char*), argp);
#else
	vsnprintf(buf, sizeof(buf), va_arg(argp, char*), argp);
#endif
	//vfprintf(dataout, va_arg(argp, char*), argp);
	va_end(argp);
	ret = send(irc_socket, buf, strlen(buf), 0);
	if(ret == -1) return 1;
	//fprintf(dataout, "\r\n");
	//return fflush(dataout);
	return 0;
}

int IRC::privmsg(char* target, char* message)
{
	if (!connected)
		return 1;

	string sout;
	sout = "PRIVMSG "; sout += target; sout += " :"; sout += message; sout += "\r\n";
	int ret = send(irc_socket, sout.c_str(), (int)sout.size(), 0);
	if(ret == -1) return 1;
	//fprintf(dataout, "PRIVMSG %s :%s\r\n", target, message);
	//return fflush(dataout);
	return 0;
}

int IRC::privmsg(char* fmt, ...)
{
	va_list argp;
//	char* target;
	
	if (!connected)
		return 1;
	//va_start(argp, fmt);
	//fprintf(dataout, "PRIVMSG %s :", fmt);
	//vfprintf(dataout, va_arg(argp, char*), argp);
	//va_end(argp);
	//fprintf(dataout, "\r\n");
	////return fflush(dataout);


	va_start(argp, fmt);
	char buf[1024];
	//sprintf(
	sprintf_s(buf, 1024, "PRIVMSG %s :", fmt);
	ret = send(irc_socket, buf, strlen(buf), 0);
	if(ret == -1) return 1;

#if LL_WINDOWS
	vsprintf_s(buf, sizeof(buf), va_arg(argp, char*), argp);
#else
	vsnprintf(buf, sizeof(buf), va_arg(argp, char*), argp);
#endif
	//vfprintf(dataout, va_arg(argp, char*), argp);
	va_end(argp);
	ret = send(irc_socket, buf, strlen(buf), 0);
	if(ret == -1) return 1;

	ret = send(irc_socket, "\r\n", strlen("\r\n"), 0);
	if(ret == -1) return 1;
	//fprintf(dataout, "\r\n");
	//return fflush(dataout);


	return 0;
}


int IRC::join(char* channel, char* channelPass)
{
	if (!connected)
		return 1;

	string sout;
	sout = "JOIN "; sout += channel; sout+= channelPass; sout += "\r\n";
	int ret = send(irc_socket, sout.c_str(), (int)sout.size(), 0);
	if(ret == -1) return 1;

	//fprintf(dataout, "JOIN %s\r\n", channel);
	
	return 0;
	//return fflush(dataout);
}

int IRC::part(char* channel)
{
	if (!connected)
		return 1;
	//fprintf(dataout, "PART %s\r\n", channel);


	string sout;
	sout = "PART "; sout += channel; sout += "\r\n";
	int ret = send(irc_socket, sout.c_str(), (int)sout.size(), 0);
	if(ret == -1) return 1;


	return 0;
	//return fflush(dataout);
}

int IRC::kick(char* channel, char* nick)
{
	if (!connected)
		return 1;
	
	string sout;
	sout = "KICK "; sout += channel; sout += " "; sout += nick; sout += "\r\n";
	int ret = send(irc_socket, sout.c_str(), (int)sout.size(), 0);
	if(ret == -1) return 1;

	//fprintf(dataout, "KICK %s %s\r\n", channel, nick);
	//return fflush(dataout);
	return 0;
}

int IRC::raw(char* data)
{
	if (!connected)
		return 1;
	//fprintf(dataout, "%s\r\n", data);

	int ret = send(irc_socket, data, strlen(data), 0);
	if(ret == -1) return 1;
	
	return 0;
	//return fflush(dataout);
}

int IRC::kick(char* channel, char* nick, char* message)
{
	if (!connected)
		return 1;
	
	sprintf_s(buf, sizeof(buf), "KICK %s %s :%s\r\n", channel, nick, message);
	int ret = send(irc_socket, buf, strlen(buf), 0);
	if(ret == -1) return 1;
	//fprintf(dataout, "KICK %s %s :%s\r\n", channel, nick, message);
	
	return 0;
	//return fflush(dataout);
}

int IRC::mode(char* channel, char* modes, char* targets)
{
	if (!connected)
		return 1;
	if (!targets)
	{
		sprintf_s(buf, sizeof(buf), "MODE %s %s\r\n", channel, modes);
		int ret = send(irc_socket, buf, strlen(buf), 0);
		if(ret == -1) return 1;
		//fprintf(dataout, "MODE %s %s\r\n", channel, modes);
	}
	else
	{
		sprintf_s(buf, sizeof(buf), "MODE %s %s %s\r\n", channel, modes, targets);
		int ret = send(irc_socket, buf, strlen(buf), 0);
		if(ret == -1) return 1;
		//fprintf(dataout, "MODE %s %s %s\r\n", channel, modes, targets);
	}
	//return fflush(dataout);
	return 0;
}

int IRC::mode(char* modes)
{
	if (!connected)
		return 1;
	mode(cur_nick, modes, 0);
	return 0;
}

int IRC::nick(const char* newnick)
{
	if (!connected)
		return 1;

	sprintf_s(buf, sizeof(buf), "NICK %s\r\n", newnick);
	int ret = send(irc_socket, buf, strlen(buf), 0);
	if(ret == -1) return 1;
	//fprintf(dataout, "NICK %s\r\n", newnick);
	//return fflush(dataout);
	return 0;
}

char* IRC::current_nick()
{
	return cur_nick;
}


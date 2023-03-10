
#include "../../headers/Server.hpp"

void  Server::handleJoinCmd(Message &msg, int senderFd)
{
	if (!_mapClients[senderFd]->getIsAuthValid())	
		errorHandler(451);
	if (msg.getArguments().empty() || msg.getArguments().at(0) == "#") /*ERR_NEEDMOREPARAMS*/
		errorHandler(461, msg.getCommand());
	if (msg.getArguments().at(0) == "#0")
	{
		leaveAllChannels(senderFd);
		return ;
	}
	checkMultiArgs(msg);
	checkChnlNames(msg.getMultiArgs());
	if (!msg.getMultiArgs().empty())
	{
		if (msg.getMultiArgs().size() > 2)/*ERR_TOOMANYTARGETS*/
			errorHandler(407, "reduce the number of targets and try the request again");
		for (size_t i = 0; i < msg.getMultiArgs().size(); i++)
		{
			if (findChannelByName(msg.getMultiArgs().at(i)))
				checkExistChannel(senderFd, msg, msg.getMultiArgs().at(i), i);
			else
			{
				if (msg.getArguments().empty())
					joinNewChannel(senderFd, msg.getMultiArgs().at(i));
				else
				{
					if (i < msg.getArguments().size())
						joinNewChannelWithKey(senderFd, msg.getMultiArgs().at(i), msg.getArguments().at(i));
					else
					{
						if (findChannelByName(msg.getMultiArgs().at(i)))
							checkExistChannel(senderFd, msg, msg.getMultiArgs().at(i), i);
						else
							joinNewChannel(senderFd, msg.getMultiArgs().at(i));
					}
				}
			}
		}
	}
	else if (msg.getMultiArgs().empty() && msg.getArguments().at(0) != "0")
	{
		if (findChannelByName(msg.getArguments().at(0)))
			checkExistChannel(senderFd, msg, msg.getArguments().at(0), 1);
		else
		{
			if (msg.getArguments().size() == 1)
				joinNewChannel(senderFd, msg.getArguments().at(0));
			else if (msg.getArguments().size() > 1)
				joinNewChannelWithKey(senderFd, msg.getArguments().at(0), msg.getArguments().at(1));
		}
	}
}

void	Server::joinNewChannel(int senderFd, std::string channelName)
{
	Channel								chnl;
	std::map<int, Client *>::iterator	it;
	std::string							rpl;
	char								hostname[256];

	it = _mapClients.find(senderFd);
	if (it->second->getJoinedChannels().size() >= (size_t)it->second->getClientMaxnumOfChannels())/*ERR_TOOMANYCHANNELS*/
		errorHandler(405, channelName);
	setChannel(chnl, channelName, it->second->getNickName());
	it->second->setJoinedChannels(channelName);
	gethostname(hostname, sizeof(hostname));
	rpl = ":" + it->second->getNickName() + "!~" + it->second->getUserName() + "@" + hostname + " JOIN :" + channelName + "\r\n"
		+ ":" + it->second->getNickName() + " MODE " + channelName + " +sn\r\n"
		+ ":irc" + " 353 " + it->second->getNickName() + " @ " + channelName + " :@" + it->second->getNickName() + "\r\n" 
		+ ":irc" + " 366 " + it->second->getNickName() + " " + channelName + " :End of /NAMES list\r\n";
	sendReplay(senderFd, rpl);
}

void	Server::setChannel(Channel &chnl, std::string channelName, std::string channelCreator)
{
	chnl.setChannelName(channelName);
	chnl.setChannelCreator(channelCreator);
	chnl.setChannelMembers(channelCreator);
	chnl.setChannelOperators(channelCreator);
	chnl.setIsMode_s(true);
	chnl.setIsMode_n(true);
	_channels.push_back(chnl);
}

void	Server::joinNewChannelWithKey(int senderFd, std::string channelName, std::string channelkey)
{
	Channel								chnl;
	std::map<int, Client *>::iterator	it;
	std::string							rpl;
	char								hostname[256];

	it = _mapClients.find(senderFd);
	if (it->second->getJoinedChannels().size() >= (size_t)it->second->getClientMaxnumOfChannels())/*ERR_TOOMANYCHANNELS*/
		errorHandler(405, channelName);
	setChannel(chnl, channelName, it->second->getNickName(), channelkey);
	it->second->setJoinedChannels(channelName);
	gethostname(hostname, sizeof(hostname));
	rpl = ":" + it->second->getNickName() + "!~" + it->second->getUserName() + "@" + hostname + " JOIN :" + channelName + "\r\n"
		+ ":" + it->second->getNickName() + " MODE " + channelName + " +kn\r\n"
		+ ":irc" + " 353 " + it->second->getNickName() + " = " + channelName + " :@" + it->second->getNickName() + "\r\n" 
		+ ":irc" + " 366 " + it->second->getNickName() + " " + channelName + " :End of /NAMES list\r\n";
	sendReplay(senderFd, rpl);
}

void	Server::setChannel(Channel &chnl, std::string channelName, std::string channelCreator,  std::string channelkey)
{
	chnl.setChannelName(channelName);
	chnl.setChannelCreator(channelCreator);
	chnl.setChannelMembers(channelCreator);
	chnl.setIsMode_s(true);
	chnl.setIsMode_n(true);
	chnl.setChannelOperators(channelCreator);
	chnl.setIsMode_k(true);
	chnl.setChannelkey(channelkey);
	_channels.push_back(chnl);
}

void	Server::checkExistChannel(int senderFd, Message &msg, std::string channelName, int i)
{
	std::map<int, Client *>::iterator	it;

	it = _mapClients.find(senderFd);
	Channel &chnl = findChannel(channelName);
	for(size_t i = 0; i < chnl.getChannelMembers().size(); i++)
	{
		if (chnl.getChannelMembers().at(i) == it->second->getNickName())
			return ;
	}
	if (it->second->getJoinedChannels().size() >= (size_t)it->second->getClientMaxnumOfChannels() )/*ERR_TOOMANYCHANNELS*/
		errorHandler(405, channelName);
	if (chnl.getIsMode_l())
	{
		if (chnl.getChannelMembers().size() >= (size_t)chnl.getChannelLimit())/*ERR_CHANNELISFULL*/
			errorHandler(471, chnl.getChannelName());
	}
	if (chnl.getIsMode_k())
	{
		if ((!msg.getMultiArgs().empty() && !msg.getArguments().empty()) || (msg.getMultiArgs().empty() && msg.getArguments().size() > 1))
		{
			if ((size_t)i < msg.getArguments().size())
			{
				if (chnl.getChannelkey() != msg.getArguments().at(i))
					errorHandler(475, chnl.getChannelName());/*ERR_BADCHANNELKEY*/
			}
		}
		else if ((!msg.getMultiArgs().empty() && msg.getArguments().empty()) || (msg.getMultiArgs().empty() && msg.getArguments().size() == 1))
			errorHandler(475, chnl.getChannelName());/*ERR_BADCHANNELKEY*/
	}
	if (chnl.getIsMode_i())
	{
		std::vector<std::string>::iterator	result = std::find(it->second->getInvitedChannels().begin() , it->second->getInvitedChannels().end(), chnl.getChannelName());
		if (result == it->second->getInvitedChannels().end())
			errorHandler(473, chnl.getChannelName());/*ERR_INVITEONLYCHAN*/
 		it->second->getInvitedChannels().erase(result);
	}
	joinExistChannel(chnl, it);
}

void    Server::joinExistChannel(Channel &chnl, std::map<int, Client *>::iterator    &it)
{
    std::string    rpl;
    std::string    namreplyFlag;
    int            fd;
    char        hostname[256];
    std::string channelMembers = "";

    chnl.setChannelMembers(it->second->getNickName());
    it->second->setJoinedChannels(chnl.getChannelName());
    
    gethostname(hostname, sizeof(hostname));
    namreplyFlag = " = ";
    if (chnl.getIsMode_s())
        namreplyFlag = " @ ";
    if (chnl.getIsMode_p())
        namreplyFlag = " * ";
    for (size_t i = 0; i < chnl.getChannelMembers().size(); i++)
    {
        if (chnl.getChannelMembers().at(i) == chnl.getChannelCreator())
            continue;
        channelMembers = channelMembers.append(" ") + chnl.getChannelMembers().at(i);
    }
    rpl = ":" + it->second->getNickName() + "!~" + it->second->getUserName() + "@" + hostname + " JOIN :" + chnl.getChannelName() + "\r\n"
        + ":" + _server_name + " 353 " + it->second->getNickName() + namreplyFlag + chnl.getChannelName() + " :" + " @" + chnl.getChannelCreator() + channelMembers + "\r\n"
        + ":" + _server_name + " 366 " + it->second->getNickName() + " " + chnl.getChannelName() + " :End of /NAMES list\r\n";
    for(size_t i = 0; i < chnl.getChannelMembers().size(); i++)
    {
        fd = findFdClientByNick(chnl.getChannelMembers().at(i));
        sendReplay(fd, rpl);
    }
}

/*** Function to leave from all channels , it works when a user type JOIN 0 ***/ 
void 	Server::leaveAllChannels(int senderFd)
{
	std::map<int, Client *>::iterator	it;
	std::string							rpl;
	int									fd;
	char								hostname[256];

	gethostname(hostname, sizeof(hostname));
	it = _mapClients.find(senderFd);
	for(size_t i = 0; i < it->second->getJoinedChannels().size(); i++)
	{
		Channel &chnl = findChannel(it->second->getJoinedChannels().at(i));
		rpl = ":" + it->second->getNickName() + "!~" + it->second->getUserName() + "@" + hostname + " PART :" + chnl.getChannelName() + "\r\n";
		for(size_t i = 0; i < chnl.getChannelMembers().size(); i++)
		{
			fd = findFdClientByNick(chnl.getChannelMembers().at(i));
			sendReplay(fd, rpl);
		}
		std::vector<std::string>::iterator	channelMember = std::find(chnl.getChannelMembers().begin(), chnl.getChannelMembers().end(), it->second->getNickName());
		chnl.getChannelMembers().erase(channelMember);
	}
	it->second->getJoinedChannels().clear();
}
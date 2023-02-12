/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ChannelCmd.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bbrahim <bbrahim@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/02/07 09:39:29 by bbrahim           #+#    #+#             */
/*   Updated: 2023/02/12 16:03:08 by bbrahim          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../headers/Server.hpp"

Channel& Server::findChannel(std::string channelName)
{
	for(size_t i = 0; i < _channels.size(); i++)
	{
		if (_channels.at(i).getChannelName() == channelName)
			return (_channels.at(i));
	}
	throw "there is no channel";
}

int	Server::findChannelByName(std::string channelName)
{
	for(size_t i = 0; i < _channels.size(); i++)
	{
		if (_channels.at(i).getChannelName() == channelName)
			return (1);
	}
	return (0);
}

void	Server::setChannel(Channel &chnl, std::string channelName, std::string channelCreator)
{
	chnl.setChannelName(channelName);
	chnl.setChannelCreator(channelCreator);
	chnl.setChannelMembers(channelCreator);
	chnl.setChannelOperators(channelCreator);
	chnl.setIsMode_o(true);
	_channels.push_back(chnl);
}

void	Server::joinNewChannel(int senderFd, std::string channelName)
{
	Channel								chnl;
	std::map<int, Client *>::iterator	it;

	it = _mapClients.find(senderFd);
	if (it->second->getJoinedChannels().size() >= (size_t)it->second->getClientMaxnumOfChannels())/*ERR_TOOMANYCHANNELS*/
		return (errorHandler(senderFd, 405, channelName));
	setChannel(chnl, channelName, it->second->getNickName());
	it->second->setJoinedChannels(channelName);
	cmd_Resp_Handler(senderFd, 353, "=", channelName, it->second->getNickName());
	cmd_Resp_Handler(senderFd, 332, channelName, "...");
}

void	Server::joinExistChannel(int senderFd, Channel &chnl, std::map<int, Client *>::iterator	&it)
{
	chnl.setChannelMembers(it->second->getNickName());
	it->second->setJoinedChannels(chnl.getChannelName());
	if (chnl.getIsMode_s())
		cmd_Resp_Handler(senderFd, 353, "@", chnl.getChannelName(), it->second->getNickName());
	else if (chnl.getIsMode_p())
		cmd_Resp_Handler(senderFd, 353, "*", chnl.getChannelName(), it->second->getNickName());
	else
		cmd_Resp_Handler(senderFd, 353, "=", chnl.getChannelName(), it->second->getNickName());
	cmd_Resp_Handler(senderFd, 332, chnl.getChannelName(), "...");
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
		return (errorHandler(senderFd, 405, channelName));
	if (chnl.getIsMode_l())
	{
		if (chnl.getChannelMembers().size() >= (size_t)chnl.getChannelLimit())/*ERR_CHANNELISFULL*/
			return (errorHandler(senderFd, 471, chnl.getChannelName()));
	}
	if (chnl.getIsMode_i())
	{
		std::vector<std::string>::iterator	result = std::find(chnl.getInvitedMembers().begin(), chnl.getInvitedMembers().end(), it->second->getNickName());
		if (result == chnl.getInvitedMembers().end())
			return (errorHandler(senderFd, 473, chnl.getChannelName()));/*ERR_INVITEONLYCHAN*/
	}
	if (chnl.getIsMode_b())
	{
		for(size_t i = 0; i < chnl.getChannelBannedMembers().size(); i++)
		{
			if (chnl.getChannelBannedMembers().at(i) == it->second->getNickName())
				return (errorHandler(senderFd, 474,  chnl.getChannelName()));/*ERR_BANNEDFROMCHAN*/
		}
	}
	if (chnl.getIsMode_s() || chnl.getIsMode_p())
	{
		if ((!msg.getMultiArgs().empty() && !msg.getArguments().empty()) || (msg.getMultiArgs().empty() && msg.getArguments().size() > 1))
		{
			if (chnl.getChannelkey() != msg.getArguments().at(i))
				return (errorHandler(senderFd, 475, chnl.getChannelName()));/*ERR_BADCHANNELKEY*/
		}
		else
			return (errorHandler(senderFd, 475, chnl.getChannelName()));/*ERR_BADCHANNELKEY*/
	}
	joinExistChannel(senderFd, chnl, it);
}

void  Server::handleJoinCmd(Message &msg, int senderFd)
{
	if (msg.getArguments().empty()) /*ERR_NEEDMOREPARAMS*/
		return (errorHandler(senderFd, 461, msg.getCommand()));
	checkMultiArgs(msg);
	checkChnlNames(msg.getMultiArgs(), senderFd);
	if (!msg.getMultiArgs().empty())
	{
		if (msg.getMultiArgs().size() > 2)/*ERR_TOOMANYTARGETS*/
			return (errorHandler(senderFd, 407, msg.getMultiArgs().at(0), "reduce the number of targets and try the request again"));
		for (size_t i = 0; i < msg.getMultiArgs().size(); i++)
		{
			if (findChannelByName(msg.getMultiArgs().at(i)))
				checkExistChannel(senderFd, msg, msg.getMultiArgs().at(i), i);
			else
				joinNewChannel(senderFd, msg.getMultiArgs().at(i));
		}
	}
	else
	{
		if (findChannelByName(msg.getArguments().at(0)))
			checkExistChannel(senderFd, msg, msg.getArguments().at(0), 1);
		else
			joinNewChannel(senderFd, msg.getArguments().at(0));
	}
}
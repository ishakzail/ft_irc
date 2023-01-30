/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bbrahim <bbrahim@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/01/29 18:23:31 by bbrahim           #+#    #+#             */
/*   Updated: 2023/01/29 18:23:50 by bbrahim          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../headers/Channel.hpp"

// Constructors
Channel::Channel()
{
	std::cout << "\e[0;33mDefault Constructor called of Channel\e[0m" << std::endl;
}

Channel::Channel(const Channel &copy)
{
	(void) copy;
	std::cout << "\e[0;33mCopy Constructor called of Channel\e[0m" << std::endl;
}

// Operators
Channel & Channel::operator=(const Channel &assign)
{
	(void) assign;
	return *this;
}

// Destructor
Channel::~Channel()
{
	std::cout << "\e[0;31mDestructor called of Channel\e[0m" << std::endl;
}
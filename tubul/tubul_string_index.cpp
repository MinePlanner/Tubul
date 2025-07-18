//
// Created by Carlos Acosta on 22-11-22.
//
#include "tubul_string_index.h"
#include <exception>
#include <iostream>

namespace TU
{

StringIndex::StringIndex(size_t bufferSize ):
	m_strId( 1000, StringIndex::PooledStringHasher{m_pool}, StringIndex::PooledStringEqual{m_pool})
{
	m_pool.reserve( bufferSize );
	m_idStr.reserve( 100 );
}

StringIndex::StringIndex():
	StringIndex(POOL_STARTING_SIZE)
{
}


StringIndex::Id StringIndex::addStr(
	std::string_view val)
{
	auto newId =  m_idStr.size();
	auto newStart = m_pool.size();
	auto newLen = val.size();
	//Actually store the string
	m_pool.insert(m_pool.end(),val.begin(),val.end());
	//Create a new string pool elem with this data, and add it to both maps
	StringPoolElement newElem{ newStart, newLen};
	m_idStr.push_back(newElem);
	m_strId.emplace(newElem,newId);

	return newId;
}

StringIndex::Id StringIndex::tryGetId(
	std::string_view val)
{
	auto found = m_strId.find(val);
	if (found != m_strId.end())
		return found->second;
	//If not found, add and return the inserted id.
	return addStr(val);
}

size_t StringIndex::getId(
	std::string_view val) const
{
	auto found = m_strId.find(val);
	if (found != m_strId.end())
		return found->second;
	throw std::runtime_error("Requesting id of unknown string" + std::string(val));
}

std::string_view StringIndex::getStr(
	size_t val_id) const
{
	if (val_id < m_idStr.size())
		return strView(m_idStr[val_id]);
	throw std::runtime_error(std::string("Requesting name of unknown id") + std::to_string(val_id));
}

void StringIndex::clear()
{
	m_pool.clear(); m_pool.shrink_to_fit();
	m_idStr.clear(); m_idStr.shrink_to_fit();
	m_strId.clear();
}

bool StringIndex::contains(const std::string_view& val) const
{
	return m_strId.contains(val);
}

size_t StringIndex::bufferCurrentSize() const
{
	return m_pool.size();
}

}

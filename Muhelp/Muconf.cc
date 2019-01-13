/* Muhelp - Helper library for Museek
 *
 * Copyright (C) 2003-2004 Hyriand <hyriand@thegraveyard.org>
 * Copyright 2008 little blue poney <lbponey@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <system.h>

#include <Muhelp/Muconf.hh>

#include <Muhelp/string_ext.hh>
#include <NewNet/nnlog.h>

#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>

namespace
{

inline void ltrim(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char ch) {
		return !std::isspace(ch);
	}));
}

inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

}

Muconf::Muconf(const std::string& filename)
        : mFilename(filename) {
	NNLOG("museek.muconf", "Muconf %s", filename.c_str());
	restore();
}

bool Muconf::hasDomain(const std::string& domain) const {
	NNLOG("museek.muconf", "hasDomain %s", domain.c_str());
	
	return mDomains.find(domain) != mDomains.end();
}

std::vector<std::string> Muconf::domains() const {
	NNLOG("museek.muconf", "domains");
	
	std::vector<std::string> d;
	
	std::map<std::string, MuconfDomain>::const_iterator it = mDomains.begin();
	for(; it != mDomains.end(); ++it)
		d.push_back((*it).first);
	
	return d;
}

MuconfDomain& Muconf::operator[](const std::string& domain) {
	NNLOG("museek.muconf", "operator[] %s", domain.c_str());
	
	std::map<std::string, MuconfDomain>::iterator it = mDomains.find(domain);
	if(it != mDomains.end())
		return (*it).second;
	
	mDomains[domain] = MuconfDomain(domain);
	
	return mDomains[domain];
}

void Muconf::restore() {
	NNLOG("museek.muconf", "restore <...>");

	MuconfDomain* cur = nullptr;
	std::ifstream f(mFilename);
	for(std::string line; std::getline(f, line);) {
		ltrim(line);
		if(line.empty())
			continue;
		if(line.front() == '#' || line.front() == ';')
			continue;

		rtrim(line);
		if(line.front() == '[' && line.back() == ']') {
			std::string domain = line.substr(1, line.size()-2);
			if(mDomains.find(domain) == mDomains.end())
				mDomains[domain] = MuconfDomain(domain);

			cur = &mDomains[domain];
		} else {
			if(!cur) {
				NNLOG("museek.muconf", "Line without domain: %s", line.c_str());
				continue;
			}

			std::string::size_type n = line.find('=');
			if(n == std::string::npos) {
				NNLOG("museek.muconf", "Line does't have '=': %s", line.c_str());
				continue;
			}
			std::string key = line.substr(0, n);
			std::string val = line.substr(n + 1);
			rtrim(key);
			ltrim(val);

			cur->restore(key, val);
		}
	}
}

void Muconf::store() {
	NNLOG("museek.muconf", "store");

	std::ofstream f(mFilename);
	for (const auto& domain : mDomains) {
		f << "[" << domain.first << "]\n";
		const std::map<std::string, std::string>& m = domain.second;
		for (const auto& kv : m)
			f << kv.first << "=" << kv.second << "\n";
		f << "\n";
	}
}

Muconf::operator std::map<std::string, std::map<std::string, std::string> >() const {
	NNLOG("museek.muconf", "operator std::map<std::string, std::map<std::string, std::string> >");
	
	std::map<std::string, std::map<std::string, std::string> > r;
	
	std::map<std::string, MuconfDomain>::const_iterator it = mDomains.begin();
	for(; it != mDomains.end(); ++it)
		r[(*it).first] = (*it).second;
	
	return r;
}

#undef MULOG_DOMAIN
#define MULOG_DOMAIN "Muconf.ND"
	
MuconfDomain::MuconfDomain(const std::string& domain)
              : mDomain(domain) {
	NNLOG("museek.muconf", "MuconfDomain <...> %s", domain.c_str());
}

bool MuconfDomain::hasKey(const std::string& key) const {
	NNLOG("museek.muconf", "hasKey %s", key.c_str());
	
	return mKeys.find(key) != mKeys.end();
}

std::vector<std::string> MuconfDomain::keys() const {
	NNLOG("museek.muconf", "keys");
	
	std::vector<std::string> d;
	
	std::map<std::string, MuconfKey>::const_iterator it = mKeys.begin();
	for(; it != mKeys.end(); ++it)
		d.push_back((*it).first);
	
	return d;
}

MuconfKey& MuconfDomain::operator[](const std::string& key) {
	NNLOG("museek.muconf", "operator[] %s", key.c_str());
	
	std::map<std::string, MuconfKey>::iterator it = mKeys.find(key);
	if(it != mKeys.end())
		return (*it).second;
	
	mKeys[key] = MuconfKey(key);
	
	return mKeys[key];
}

void MuconfDomain::restore(const std::string& key, const std::string& val) {
	if(mKeys.find(key) == mKeys.end())
		mKeys[key] = MuconfKey(key);

	mKeys[key] = val;
}

MuconfDomain::operator std::map<std::string, std::string>() const {
	NNLOG("museek.muconf", "operator std::map<std::string, std::string>");
	
	std::map<std::string, std::string> r;
	
	std::map<std::string, MuconfKey>::const_iterator it = mKeys.begin();
	for(; it != mKeys.end(); ++it)
		r[(*it).first] = (const char*)(*it).second;
	
	return r;
}

std::string MuconfDomain::domain() const {
	NNLOG("museek.muconf", "domain");
	
	return mDomain;
}

void MuconfDomain::remove(const std::string& key) {
	NNLOG("museek.muconf", "remove %s", key.c_str());
	
	mKeys.erase(key);
}

#undef MULOG_DOMAIN
#define MULOG_DOMAIN "Muconf.NK"

MuconfKey::MuconfKey(const std::string& key)
           : mKey(key) {
	NNLOG("museek.muconf", "MuconfKey::MuconfKey <...> %s", mKey.c_str());
}

void MuconfKey::operator=(const std::string& value) {
	NNLOG("museek.muconf", "MuconfKey::operator= %s", value.c_str());
	
	mValue = value;
}

void MuconfKey::operator=(const char* value) {
	NNLOG("museek.muconf", "MuconfKey::operator= %s", value);
	
	mValue = std::string(value);
}

void MuconfKey::operator=(uint value) {
	NNLOG("museek.muconf", "MuconfKey::operator= %u", value);
	
	char x[80];
	snprintf(x, 80, "%u", value);
	mValue = x;
}

void MuconfKey::operator=(int value) {
	NNLOG("museek.muconf", "MuconfKey::operator= %i", value);
	
	char x[80];
	snprintf(x, 80, "%i", value);
	mValue = x;
}

void MuconfKey::operator=(double value) {
	NNLOG("museek.muconf", "MuconfKey::operator= %f", value);
	
	char x[80];
	snprintf(x, 80, "%f", value);
	mValue = x;
}

void MuconfKey::operator=(bool value) {
	NNLOG("museek.muconf", "MuconfKey::operator= %d", value);
	if(value)
		mValue = "true";
	else
		mValue = "false";
}

MuconfKey::operator std::string() const {
	NNLOG("museek.muconf", "MuconfKey::operator std::string");
	
	return mValue;
}

MuconfKey::operator const char*() const {
	NNLOG("museek.muconf", "MuconfKey::operator const char*");
	
	return mValue.c_str();
}

uint MuconfKey::asUint() const {
	NNLOG("museek.muconf", "MuconfKey::asUint");
	
	return atol(mValue.c_str());
}

int MuconfKey::asInt() const {
	NNLOG("museek.muconf", "MuconfKey::asInt");

	return atoi(mValue.c_str());
}

double MuconfKey::asDouble() const {
	NNLOG("museek.muconf", "MuconfKey::asDouble");
	
	return strtod(mValue.c_str(), NULL);
}

bool MuconfKey::asBool() const {
	NNLOG("museek.muconf", "MuconfKey::asBool");
	
	if(tolower(mValue) == "true" || asInt() != 0)
		return true;
	else
		return false;
}

bool MuconfKey::operator==(const std::string& v) const {
	NNLOG("museek.muconf", "MuconfKey::operator== %s", v.c_str());
	
	return mValue == v;
}

bool MuconfKey::operator!=(const std::string& v) const {
	NNLOG("museek.muconf", "MuconfKey::operator!= %s", v.c_str());
	
	return mValue != v;
}

bool MuconfKey::operator!() const {
	return mValue.size() == 0;
}

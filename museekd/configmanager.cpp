/*  Museek - A SoulSeek client written in C++
    Copyright (C) 2006-2007 Ingmar K. Steen (iksteen@gmail.com)
    Copyright 2008 little blue poney <lbponey@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif // HAVE_CONFIG_H
#include "configmanager.h"
#include <NewNet/nnlog.h>
#include <cstring>
#include <fstream>

namespace
{

inline void
ltrim(std::string& s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char ch) {
    return !std::isspace(ch);
  }));
}

inline void
rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](char ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

}

Museek::ConfigManager::ConfigManager() : m_AutoSave(true)
{
}

bool
Museek::ConfigManager::load(const std::string & path)
{
  NNLOG("museekd.config.debug", "Loading configuration '%s'.", path.c_str());

  // Load and parse the specified configuration file.
  std::ifstream f(path);
  if(!f.is_open())
  {
    NNLOG("museekd.config.warn", "Could not open configuration file '%s'.", path.c_str());
    return false;
  }

  // Store the path so we can auto-save changes.
  m_Path = path;
  // While loading the configuration, disable autosave to prevent spurious saves.
  bool oldAutoSave = m_AutoSave;
  setAutoSave(false);

  std::string domain;
  int number = 0;
  for(std::string line; std::getline(f, line);) {
    ++number;
    ltrim(line);
    if(line.empty())
      continue;
    if(line.front() == '#' || line.front() == ';')
      continue;

    rtrim(line);
    if(line.front() == '[' && line.back() == ']') {
      // Get the domain from section string
      domain = line.substr(1, line.size()-2);
    }
    else
    {
      // Otherwise add key=val pair
      if(domain.empty()) {
        NNLOG("museekd.config.warn", "Line %d without domain: %s", number, line.c_str());
      }
      std::string::size_type n = line.find('=');
      if(n == std::string::npos) {
        NNLOG("museek.muconf", "Line %d does't have '=': %s", number, line.c_str());
        continue;
      }
      std::string key = line.substr(0, n);
      std::string val = line.substr(n + 1);
      rtrim(key);
      ltrim(val);

      set(domain, key, val);
    }
  }

  // Restore the auto-save state.
  setAutoSave(oldAutoSave);

  return true;
}

bool
Museek::ConfigManager::save(const std::string & path) const
{
  // Check if we know where to save the configuration.
  const std::string& dest = path.empty() ? m_Path : path;
  if(dest.empty())
  {
    NNLOG("museekd.config.warn", "No path to save configuration to specified.");
    return false;
  }

  NNLOG("museekd.config.config.debug", "Saving configuration to '%s'.", dest.c_str());

  std::ofstream f(dest);
  if(!f.is_open())
  {
    NNLOG("museekd.config.warn", "Unable to save configuration to '%s'.", dest.c_str());
    return false;
  }

  for(const auto& domain : m_Config)
  {
    f << "[" << domain.first << "]\n";
    for (const auto& kv : domain.second)
      f << kv.first << "=" << kv.second << "\n";
    f << "\n";
  }
  return true;
}

std::string
Museek::ConfigManager::get(const std::string & domain, const std::string & key, const std::string & defaultValue) const
{
  // Try to find the domain.
  Config::const_iterator it = m_Config.find(domain);
  if(it == m_Config.end())
    return defaultValue; // Domain not found, return default value.
  // Try to find the key.
  Domain::const_iterator it2 = (*it).second.find(key);
  if(it2 == (*it).second.end())
    return defaultValue; // Key not found, return default value.
  // Return the key's value.
  return (*it2).second;
}

unsigned int
Museek::ConfigManager::getUint(const std::string & domain, const std::string & key, unsigned int defaultValue) const
{
  // Get the string value from the configuration.
  std::string value(get(domain, key));
  if(value.empty())
    return defaultValue; // No such value, return the default value.
  return atol(value.c_str()); // Convert the string to an unsigned integer value.
}

int
Museek::ConfigManager::getInt(const std::string & domain, const std::string & key, int defaultValue) const
{
  // Get the string value from the configuration.
  std::string value(get(domain, key));
  if(value.empty())
    return defaultValue; // No such value, return the default value.
  return atol(value.c_str()); // Convert the string to a signed integer value.
}

double
Museek::ConfigManager::getDouble(const std::string & domain, const std::string & key, double defaultValue) const
{
  // Get the string value from the configuration.
  std::string value(get(domain, key));
  if(value.empty())
    return defaultValue; // No such value, return the default value.
  return strtod(value.c_str(), 0); // Convert the string to a double value.
}

bool
Museek::ConfigManager::getBool(const std::string & domain, const std::string & key, bool defaultValue) const
{
  // Get the string value from the configuration.
  std::string value(get(domain, key));
  if(value.empty())
    return defaultValue; // No such value, return the default value.
  return value == "true"; // If the value is 'true' return true, false otherwise.
}

void
Museek::ConfigManager::set(const std::string & domain, const std::string & key, const std::string & value)
{
  // Set the value in the configuration.
  m_Config[domain][key] = value;
  // Save the configuration if auto-save is enabled.
  doAutoSave();
  // Emit the key set / changed event.
  emitKeySetEvent(domain, key, value);
}

void
Museek::ConfigManager::set(const std::string & domain, const std::string & key, const char * value)
{
  // Set the value in the configuration.
  m_Config[domain][key] = value;
  // Save the configuration if auto-save is enabled.
  doAutoSave();
  // Emit the key set / changed event.
  emitKeySetEvent(domain, key, value);
}

void
Museek::ConfigManager::set(const std::string & domain, const std::string & key, unsigned int value)
{
  // Format the unsigned integer to a string value.
  char x[80];
  snprintf(x, 80, "%u", value);
  // Set the value in the configuration.
  m_Config[domain][key] = x;
  // Save the configuration if auto-save is enabled.
  doAutoSave();
  // Emit the key set / changed event.
  emitKeySetEvent(domain, key, x);
}

void
Museek::ConfigManager::set(const std::string & domain, const std::string & key, int value)
{
  // Format the unsigned integer to a string value.
  char x[80];
  snprintf(x, 80, "%i", value);
  // Set the value in the configuration.
  m_Config[domain][key] = x;
  // Save the configuration if auto-save is enabled.
  doAutoSave();
  // Emit the key set / changed event.
  emitKeySetEvent(domain, key, x);
}

void
Museek::ConfigManager::set(const std::string & domain, const std::string & key, double value)
{
  // Format the unsigned integer to a string value.
  char x[80];
  snprintf(x, 80, "%f", value);
  // Set the value in the configuration.
  m_Config[domain][key] = x;
  // Save the configuration if auto-save is enabled.
  doAutoSave();
  // Emit the key set / changed event.
  emitKeySetEvent(domain, key, x);
}

void
Museek::ConfigManager::set(const std::string & domain, const std::string & key, bool value)
{
  // Set the value in the configuration.
  m_Config[domain][key] = value ? "true" : "false";
  // Save the configuration if auto-save is enabled.
  doAutoSave();
  // Emit the key set / changed event.
  emitKeySetEvent(domain, key, value ? "true" : "false");
}

void
Museek::ConfigManager::removeKey(const std::string & domain, const std::string & key)
{
  // Check if the domain exists.
  Config::iterator it = m_Config.find(domain);
  if(it == m_Config.end())
    return; // Nope, nothing to do.
  // Check if the key exists in the domain.
  Domain::iterator it2 = (*it).second.find(key);
  if(it2 == (*it).second.end())
    return; // Nope, nothing to do.
  // Remove the key (and its value) from the configuration.
  (*it).second.erase(it2);

  // Save the configuration if auto-save is enabled.
  doAutoSave();

  // Prepare key-removed event data.
  RemoveNotify data;
  data.domain = domain;
  data.key = key;
  // Emit the key removed event.
  keyRemovedEvent(&data);
}

bool
Museek::ConfigManager::hasDomain(const std::string & domain) const
{
  // Check if the domain exists.
  return m_Config.find(domain) != m_Config.end();
}

bool
Museek::ConfigManager::hasKey(const std::string & domain, const std::string & key) const
{
  // Check if the domain exists.
  Config::const_iterator it = m_Config.find(domain);
  if(it == m_Config.end())
    return false; // Nope, key can't exist either.
  // Check if the key exists.
  return (*it).second.find(key) != (*it).second.end();
}

std::vector<std::string>
Museek::ConfigManager::keys(const std::string & domain) const
{
  std::vector<std::string> result;

  // Check if the domain exists.
  Config::const_iterator it(m_Config.find(domain));
  if(it == m_Config.end())
    return result; // Nope, no keys without a domain.

  // Iterate over the domain's keys.
  Domain::const_iterator it2, end2((*it).second.end());
  for(it2 = (*it).second.begin(); it2 != end2; ++it2)
    result.push_back((*it2).first); // And push them on the list.

  // Return the result.
  return result;
}

void
Museek::ConfigManager::emitKeySetEvent(const std::string & domain, const std::string & key, const std::string & value)
{
  // Set up the event's data object.
  ChangeNotify data;
  data.domain = domain;
  data.key = key;
  data.value = value;
  // Emit the key set event.
  keySetEvent(&data);
}

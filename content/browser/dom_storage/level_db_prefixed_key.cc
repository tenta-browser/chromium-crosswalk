/*
 * level_db_prefixed_key.cc
 *
 *  Created on: Dec 14, 2018
 *      Author: iotto
 */

#include "content/browser/dom_storage/level_db_prefixed_key.h"

#include "url/gurl.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace content {

namespace {
// TODO(iotto): Copied from 'content/browser/dom_storage/local_storage_context_mojo.cc'
const char kOriginSeparator = '\x00';
//const char kDataPrefix[] = "_";
const char kPrefixSeparator = ':';
} // namespace

LevelDBPrefixedKey::LevelDBPrefixedKey(const std::string& prefix, const url::Origin& origin)
    : _embedder_prefix(prefix),
      _origin(origin) {
  // domain:embedderPrefix:origin\0
  _prefixed_key = GetEffectiveDomain(origin.GetURL().host()) + kPrefixSeparator + _embedder_prefix + kPrefixSeparator
                  + _origin.Serialize() + kOriginSeparator;
  // TODO(iotto) :remove these commented out
//  _prefixed_key = _embedder_prefix + kPrefixSeparator + _origin.Serialize() + kPrefixSeparator;
  //kOriginSeparator;
}

LevelDBPrefixedKey::LevelDBPrefixedKey(const LevelDBPrefixedKey& other)
    : _embedder_prefix(other._embedder_prefix),
      _origin(other._origin),
      _prefixed_key(other._prefixed_key) {
}

// input = domain:embedderPrefix:origin\0
int LevelDBPrefixedKey::ParsePrefix(const std::string& input) {
  // TODO(iotto): See how to handle old format! (better delete all!)
  size_t separator = input.find(kPrefixSeparator);
  if ( separator == std::string::npos ) {
    LOG(ERROR) << __func__ << " domain_not_found_in=" << input;
    return -1;
  }
  // got the domain, ignore since origin has it

  separator = input.find(kPrefixSeparator, separator);
  if ( separator == std::string::npos ) {
    LOG(ERROR) << __func__ << " prefix_not_found_in=" << input;
    return -1;
  }
  // got embedder_prefix

  _embedder_prefix = input.substr(0, separator);
  _origin = url::Origin::Create(GURL(input.substr(separator+1)));

  _prefixed_key = input;
  LOG(INFO) << "iotto " << __func__ << " prefix=" << _embedder_prefix
      << " origin=" << _origin.Serialize() << " input=" << input;

  return 0;
}

std::string LevelDBPrefixedKey::GetEffectiveDomain(const std::string& urlHost) {
  using namespace net::registry_controlled_domains;

  std::string effective_domain(GetDomainAndRegistry(urlHost, INCLUDE_PRIVATE_REGISTRIES));
  if (effective_domain.empty()) {
    effective_domain = urlHost;
  }

  if (!effective_domain.empty() && effective_domain[0] == '.') {
    return effective_domain.substr(1);
  }
  return effective_domain;
}

} /* namespace content */

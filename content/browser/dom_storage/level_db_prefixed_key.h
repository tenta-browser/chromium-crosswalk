/*
 * level_db_prefixed_key.h
 *
 *  Created on: Dec 14, 2018
 *      Author: iotto
 */

#ifndef CONTENT_BROWSER_DOM_STORAGE_LEVEL_DB_PREFIXED_KEY_H_
#define CONTENT_BROWSER_DOM_STORAGE_LEVEL_DB_PREFIXED_KEY_H_

#include <string>

#include "url/origin.h"

namespace content {

class LevelDBPrefixedKey {
 public:
  LevelDBPrefixedKey() = default;
  LevelDBPrefixedKey(const std::string& prefix, const url::Origin& origin);
  LevelDBPrefixedKey(const LevelDBPrefixedKey& other);

  bool operator==(const LevelDBPrefixedKey& other) const {
    return _prefixed_key == other._prefixed_key;
  }

  bool operator<(const LevelDBPrefixedKey& other) const {
    return _prefixed_key.compare(other._prefixed_key);
  }
  const std::string& GetPrefix() const {
    return _prefixed_key;
  }

  const std::string& embedder_prefix() const {
    return _embedder_prefix;
  }
  const url::Origin& origin() const {
    return _origin;
  }

  int ParsePrefix(const std::string& input);

 private:
  std::string GetEffectiveDomain(const std::string& urlHost);

  // TODO(iotto): Leave these out
  std::string _embedder_prefix;
  url::Origin _origin;

  std::string _prefixed_key;
};

} /* namespace content */

#endif /* CONTENT_BROWSER_DOM_STORAGE_LEVEL_DB_PREFIXED_KEY_H_ */

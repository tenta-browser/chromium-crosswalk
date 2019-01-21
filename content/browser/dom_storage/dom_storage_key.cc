/*
 * dom_storage_key.cc
 *
 *  Created on: Jan 15, 2019
 *  Author: iotto
 */

#include "content/browser/dom_storage/dom_storage_key.h"
#include "url/gurl.h"

namespace content {

const char DOMStorageKey::kOriginSeparator = '\x00';
const char DOMStorageKey::kDataPrefix[] = "_";

DOMStorageKey::DOMStorageKey(const url::Origin& origin)
    : _origin(origin) {
  GenerateKey();
}

DOMStorageKey::DOMStorageKey(const DOMStorageKey& other)
    : _origin(other._origin),
      _key(other._key) {
}

void DOMStorageKey::GenerateKey() {
  _key = kDataPrefix + _origin.Serialize() + kOriginSeparator;
}

std::string DOMStorageKey::GetKey() const {
  return _key;
}

bool DOMStorageKey::operator ==(const DOMStorageKey& other) const {
  return _key == other._key;
}

bool DOMStorageKey::operator <(const DOMStorageKey& other) const {
  return (_key < other._key);
}

int DOMStorageKey::ParseKey(const std::string& input) {
  if ( input.empty() || input.length() <= 1) { // we should have kDataPrefix at least
    return -1;  // TODO(iotto): Return some meaningfull error
  }

  _origin = url::Origin::Create(GURL(input.substr(1, input.length()-1)));
  _key = input;

  LOG(INFO) << "iotto " << __func__ << " origin=" << _origin.Serialize() << " input=" << input;
  return 0;
}
} /* namespace content */

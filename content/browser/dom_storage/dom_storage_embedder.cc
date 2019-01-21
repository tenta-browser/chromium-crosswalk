/*
 * dom_storage_embedder.cc
 *
 *  Created on: Jan 15, 2019
 *      Author: iotto
 */

#include "content/browser/dom_storage/dom_storage_embedder.h"

#include "content/browser/dom_storage/dom_storage_key.h"

using url::Origin;

namespace content {

DOMStorageEmbedder::DOMStorageEmbedder() {
  // TODO Auto-generated constructor stub

}

DOMStorageEmbedder::~DOMStorageEmbedder() {
  // TODO Auto-generated destructor stub
}

scoped_refptr<content::DOMStorageKey> DOMStorageEmbedder::KeyFromOrigin(const Origin& fromOrigin) {
  return new DOMStorageKey(fromOrigin);
}

scoped_refptr<content::DOMStorageKey> DOMStorageEmbedder::DecodeKey(const std::string& fromString) {
  scoped_refptr<content::DOMStorageKey> dom_key(new DOMStorageKey());
  if ( dom_key->ParseKey(fromString) == 0) {
    LOG(ERROR) << __func__ << " Invalid DOMKey=" << fromString;
  }

  return dom_key;
}

} /* namespace content */

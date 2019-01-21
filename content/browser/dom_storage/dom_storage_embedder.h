/*
 * dom_storage_embedder.h
 *
 *  Created on: Jan 15, 2019
 *  Author: iotto
 */

#ifndef CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_EMBEDDER_H_
#define CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_EMBEDDER_H_

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "url/origin.h"

namespace content {

class DOMStorageKey;

// TODO(iotto): Move to content/browser/public
class DOMStorageEmbedder :
    public base::RefCountedThreadSafe<DOMStorageEmbedder> {
 public:
  DOMStorageEmbedder();

  virtual scoped_refptr<content::DOMStorageKey> KeyFromOrigin(const url::Origin& fromOrigin);
  virtual scoped_refptr<content::DOMStorageKey> DecodeKey(const std::string& fromString);
 protected:
  friend class base::RefCountedThreadSafe<DOMStorageEmbedder>;
  virtual ~DOMStorageEmbedder();
};

} /* namespace content */

#endif /* CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_EMBEDDER_H_ */

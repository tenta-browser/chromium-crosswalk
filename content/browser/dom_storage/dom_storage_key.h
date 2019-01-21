/*
 * dom_storage_key.h
 *
 *  Created on: Jan 15, 2019
 *      Author: iotto
 */

#ifndef CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_KEY_H_
#define CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_KEY_H_

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "url/origin.h"

namespace content {

class CONTENT_EXPORT DOMStorageKey :
    public base::RefCountedThreadSafe<DOMStorageKey> {
 public:
  DOMStorageKey() = default;
  DOMStorageKey(const url::Origin& origin);
  DOMStorageKey(const DOMStorageKey& other);

  virtual std::string GetKey() const;
  virtual int ParseKey(const std::string& input);

  virtual bool operator==(const DOMStorageKey& other) const;
  virtual bool operator<(const DOMStorageKey& other) const;

  const url::Origin& origin() const {
    return _origin;
  }

 protected:
  friend class base::RefCountedThreadSafe<DOMStorageKey>;
  virtual ~DOMStorageKey() = default;

 private:
  void GenerateKey();

 protected:
  url::Origin _origin;
  std::string _key;  // generated key

  static const char kOriginSeparator;
  static const char kDataPrefix[];
};

}
/* namespace content */

#endif /* CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_KEY_H_ */

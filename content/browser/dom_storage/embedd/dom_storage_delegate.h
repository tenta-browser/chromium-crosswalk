/*
 * dom_storage_delegate.h
 *
 *  Created on: Jan 15, 2019
 *  Author: iotto
 */

#ifndef CONTENT_BROWSER_DOM_STORAGE_EMBEDD_DOM_STORAGE_DELEGATE_H_
#define CONTENT_BROWSER_DOM_STORAGE_EMBEDD_DOM_STORAGE_DELEGATE_H_

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"

namespace content {
namespace embedd {

class CONTENT_EXPORT DOMStorageDelegate :
    public base::RefCountedThreadSafe<DOMStorageDelegate> {
 public:
  DOMStorageDelegate();

 protected:
  friend class base::RefCountedThreadSafe<DOMStorageDelegate>;
  virtual ~DOMStorageDelegate();
};

} /* namespace embedd */
} /* namespace content */

#endif /* CONTENT_BROWSER_DOM_STORAGE_EMBEDD_DOM_STORAGE_DELEGATE_H_ */

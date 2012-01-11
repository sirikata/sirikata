// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#ifndef _SIRIKATA_LIBSPACE_SPACE_MODULE_HPP_
#define _SIRIKATA_LIBSPACE_SPACE_MODULE_HPP_

#include <sirikata/space/Platform.hpp>
#include <sirikata/core/service/Service.hpp>
#include <sirikata/space/SpaceContext.hpp>
#include <sirikata/core/util/Factory.hpp>

namespace Sirikata {

/** A SpaceModule is a generic service for a space. It allows you to extend the
 *  basic fixed functionality of the space by hooking into it's components
 *  via the SpaceContext. Any sort of functionality could be implemented here,
 *  e.g. a service for custom global settings that are relayed to objects or
 *  object hosts, an HTTP control interface, or a service to track per-user
 *  state such as inventory or game statistics.
 *
 *  Access to the space server is provided through the SpaceModule, so no
 *  additional interface is required beyond implementing Service, which tells
 *  the SpaceModule when to start and stop its services.
 */
class SIRIKATA_SPACE_EXPORT SpaceModule : public Service {
  public:
    SpaceModule(SpaceContext* ctx)
        : mContext(ctx)
    {}
    virtual ~SpaceModule();

  protected:
    SpaceContext* mContext;
}; // class SpaceModuleFactory


class SIRIKATA_SPACE_EXPORT SpaceModuleFactory
    : public AutoSingleton<SpaceModuleFactory>,
      public Factory2<SpaceModule*, SpaceContext*, const String&>
{
  public:
    static SpaceModuleFactory& getSingleton();
    static void destroy();
}; // class SpaceModuleFactory


} // namespace Sirikata

#endif //_SIRIKATA_LIBSPACE_SPACE_MODULE_HPP_

/**    @file ESTFConcurrencyComposite.h
 *    @brief ESTFConcurrencyComposites contain many ESTFComponents and run them
 *      concurrently.
 *
 * Copyright (c) 2009 Yahoo! Inc.
 * The copyrights embodied in the content of this file are licensed by Yahoo! Inc.
 * under the BSD (revised) open source license.
 *
 * Derived from code that is Copyright (c) 2009 Joshua Blatt and offered under both
 * BSD and Apache 2.0 licenses (http://sourceforge.net/projects/sparrowhawk/).
 *
 *  $Author: blattj $
 *  $Date: 2009/05/25 21:51:19 $
 *  $Name:  $
 *  $Revision: 1.3 $
 */

#ifndef ESTF_CONCURRENCY_COMPOSITE_H
#define ESTF_CONCURRENCY_COMPOSITE_H

#ifndef ESTF_CONFIG_H
#include <ESTFConfig.h>
#endif

#ifndef ESTF_COMPOSITE_H
#include <ESTFComposite.h>
#endif

#ifndef ESTF_RESULT_COLECTOR_H
#include <ESTFResultCollector.h>
#endif

#ifndef ESTF_COMPONENT_H
#include <ESTFComponent.h>
#endif

#ifndef ESTF_OBJECT_PTR_H
#include <ESTFObjectPtr.h>
#endif

#include <list>

/** ESTFConcurrencyComposites contain many ESTFComponents and run them
 *  concurrently.  When the its run method is called, it will create new
 *  threads to call each each child's run method in different threads of
 *  control.
 *
 *  @ingroup test
 */
class ESTFConcurrencyComposite : public ESTFComposite
{
public:
    /** Default Constructor. */
    ESTFConcurrencyComposite();

    /** Destructor.
     */
    virtual ~ESTFConcurrencyComposite();

    /** Run the component.  This will create new threads to run each child
     *  component in a different thread of control.
     *
     *    @param collector A result collector that will collect the results of
     *        this test run.
     *    @return true if the test run was successfully performed by the test
     *        framework.  Application errors discovered during a test run do not
     *        count, a false return means there was an error in the test suite
     *        itself that prevented it from completing one or more test cases.
     */
    virtual bool run( ESTFResultCollector *collector );

    /** Returns a deep copy of the component.  This will return a deep copy
     *    of this composite and deep copy of every child component in the
     *    composite.
     *
     *    @return A deep copy of the composite.
     */
    virtual ESTFComponentPtr clone();
};

DEFINE_ESTF_OBJECT_PTR(ESTFConcurrencyComposite,ESTFComposite);

#endif                                 /* ! ESTF_CONCURRENCY_COMPOSITE_H */

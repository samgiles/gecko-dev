/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is 
 * Crocodile Clips Ltd..
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#ifndef __NS_SVGVALUE_H__
#define __NS_SVGVALUE_H__

#include "nscore.h"
#include "nsISVGValue.h"
#include "nsGenericElement.h" // for CheapVoidArray
#include "nsISVGValueObserver.h"

class nsSVGValue : public nsISVGValue
{
protected:
  nsSVGValue();
  virtual ~nsSVGValue();

  // to be called by subclass whenever value is being modified.
  // nested calls will be ignored, so calls need to be balanced
  void WillModify();
  void DidModify();
  
public:
  // Partial Implementation of nsISVGValue interface:
  NS_IMETHOD AddObserver(nsISVGValueObserver* observer);
  NS_IMETHOD RemoveObserver(nsISVGValueObserver* observer);

  typedef
  NS_STDCALL_FUNCPROTO(nsresult,
                       SVGObserverNotifyFunction,
                       nsISVGValueObserver, DidModifySVGObservable,
                       (nsISVGValue*));

  
protected:
  // implementation helpers
  void ReleaseObservers();
  
  void NotifyObservers(SVGObserverNotifyFunction f);
  
private:
  nsSmallVoidArray mObservers;
  PRInt32 mModifyNestCount;
};


#endif //__NS_SVGVALUE_H__

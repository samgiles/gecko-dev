/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_TextEvents_h__
#define mozilla_TextEvents_h__

#include <stdint.h>

#include "mozilla/Assertions.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/EventForwards.h" // for KeyNameIndex, temporarily
#include "mozilla/TextRange.h"
#include "nsCOMPtr.h"
#include "nsIDOMKeyEvent.h"
#include "nsITransferable.h"
#include "nsRect.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

/******************************************************************************
 * virtual keycode values
 ******************************************************************************/

#define NS_DEFINE_VK(aDOMKeyName, aDOMKeyCode) NS_##aDOMKeyName = aDOMKeyCode

enum
{
#include "nsVKList.h"
};

#undef NS_DEFINE_VK

#define kLatestSeqno UINT32_MAX

namespace mozilla {

namespace dom {
  class PBrowserParent;
  class PBrowserChild;
} // namespace dom
namespace plugins {
  class PPluginInstanceChild;
} // namespace plugins

/******************************************************************************
 * mozilla::AlternativeCharCode
 *
 * This stores alternative charCode values of a key event with some modifiers.
 * The stored values proper for testing shortcut key or access key.
 ******************************************************************************/

struct AlternativeCharCode
{
  AlternativeCharCode(uint32_t aUnshiftedCharCode, uint32_t aShiftedCharCode) :
    mUnshiftedCharCode(aUnshiftedCharCode), mShiftedCharCode(aShiftedCharCode)
  {
  }
  uint32_t mUnshiftedCharCode;
  uint32_t mShiftedCharCode;
};

/******************************************************************************
 * mozilla::WidgetKeyboardEvent
 ******************************************************************************/

class WidgetKeyboardEvent : public WidgetInputEvent
{
private:
  friend class dom::PBrowserParent;
  friend class dom::PBrowserChild;

  WidgetKeyboardEvent()
  {
  }

public:
  virtual WidgetKeyboardEvent* AsKeyboardEvent() MOZ_OVERRIDE { return this; }

  WidgetKeyboardEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget) :
    WidgetInputEvent(aIsTrusted, aMessage, aWidget, NS_KEY_EVENT),
    keyCode(0), charCode(0),
    location(nsIDOMKeyEvent::DOM_KEY_LOCATION_STANDARD),
    isChar(false), mIsRepeat(false),
    mKeyNameIndex(mozilla::KEY_NAME_INDEX_Unidentified),
    mNativeKeyEvent(nullptr),
    mUniqueId(0)
  {
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    MOZ_ASSERT(eventStructType == NS_KEY_EVENT,
               "Duplicate() must be overridden by sub class");
    // Not copying widget, it is a weak reference.
    WidgetKeyboardEvent* result =
      new WidgetKeyboardEvent(false, message, nullptr);
    result->AssignKeyEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  // A DOM keyCode value or 0.  If a keypress event whose charCode is 0, this
  // should be 0.
  uint32_t keyCode;
  // If the instance is a keypress event of a printable key, this is a UTF-16
  // value of the key.  Otherwise, 0.  This value must not be a control
  // character when some modifiers are active.  Then, this value should be an
  // unmodified value except Shift and AltGr.
  uint32_t charCode;
  // One of nsIDOMKeyEvent::DOM_KEY_LOCATION_*
  uint32_t location;
  // OS translated Unicode chars which are used for accesskey and accelkey
  // handling. The handlers will try from first character to last character.
  nsTArray<AlternativeCharCode> alternativeCharCodes;
  // Indicates whether the event signifies a printable character
  bool isChar;
  // Indicates whether the event is generated by auto repeat or not.
  // if this is keyup event, always false.
  bool mIsRepeat;
  // DOM KeyboardEvent.key
  KeyNameIndex mKeyNameIndex;
  // DOM KeyboardEvent.key only when mKeyNameIndex is KEY_NAME_INDEX_USE_STRING.
  nsString mKeyValue;
  // OS-specific native event can optionally be preserved
  void* mNativeKeyEvent;
  // Unique id associated with a keydown / keypress event. Used in identifing
  // keypress events for removal from async event dispatch queue in metrofx
  // after preventDefault is called on keydown events. It's ok if this wraps
  // over long periods.
  uint32_t mUniqueId;

  void GetDOMKeyName(nsAString& aKeyName)
  {
    if (mKeyNameIndex == KEY_NAME_INDEX_USE_STRING) {
      aKeyName = mKeyValue;
      return;
    }
    GetDOMKeyName(mKeyNameIndex, aKeyName);
  }

  static void GetDOMKeyName(mozilla::KeyNameIndex aKeyNameIndex,
                            nsAString& aKeyName);

  void AssignKeyEventData(const WidgetKeyboardEvent& aEvent, bool aCopyTargets)
  {
    AssignInputEventData(aEvent, aCopyTargets);

    keyCode = aEvent.keyCode;
    charCode = aEvent.charCode;
    location = aEvent.location;
    alternativeCharCodes = aEvent.alternativeCharCodes;
    isChar = aEvent.isChar;
    mIsRepeat = aEvent.mIsRepeat;
    mKeyNameIndex = aEvent.mKeyNameIndex;
    mKeyValue = aEvent.mKeyValue;
    // Don't copy mNativeKeyEvent because it may be referred after its instance
    // is destroyed.
    mNativeKeyEvent = nullptr;
    mUniqueId = aEvent.mUniqueId;
  }
};

/******************************************************************************
 * mozilla::WidgetTextEvent
 *
 * XXX WidgetTextEvent is fired with compositionupdate event almost every time.
 *     This wastes performance and the cost of mantaining each platform's
 *     implementation.  Therefore, we should merge WidgetTextEvent and
 *     WidgetCompositionEvent.  Then, DOM compositionupdate should be fired
 *     from TextComposition automatically.
 ******************************************************************************/

class WidgetTextEvent : public WidgetGUIEvent
{
private:
  friend class dom::PBrowserParent;
  friend class dom::PBrowserChild;
  friend class plugins::PPluginInstanceChild;

  WidgetTextEvent()
    : mSeqno(kLatestSeqno)
    , isChar(false)
  {
  }

public:
  uint32_t mSeqno;

public:
  virtual WidgetTextEvent* AsTextEvent() MOZ_OVERRIDE { return this; }

  WidgetTextEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_TEXT_EVENT)
    , mSeqno(kLatestSeqno)
    , isChar(false)
  {
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    MOZ_ASSERT(eventStructType == NS_TEXT_EVENT,
               "Duplicate() must be overridden by sub class");
    // Not copying widget, it is a weak reference.
    WidgetTextEvent* result = new WidgetTextEvent(false, message, nullptr);
    result->AssignTextEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  // The composition string or the commit string.
  nsString theText;
  // Indicates whether the event signifies printable text.
  // XXX This is not a standard, and most platforms don't set this properly.
  //     So, perhaps, we can get rid of this.
  bool isChar;

  nsRefPtr<TextRangeArray> mRanges;

  void AssignTextEventData(const WidgetTextEvent& aEvent, bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    isChar = aEvent.isChar;

    // Currently, we don't need to copy the other members because they are
    // for internal use only (not available from JS).
  }

  bool IsComposing() const
  {
    return mRanges && mRanges->IsComposing();
  }

  uint32_t TargetClauseOffset() const
  {
    return mRanges ? mRanges->TargetClauseOffset() : 0;
  }

  uint32_t RangeCount() const
  {
    return mRanges ? mRanges->Length() : 0;
  }
};

/******************************************************************************
 * mozilla::WidgetCompositionEvent
 ******************************************************************************/

class WidgetCompositionEvent : public WidgetGUIEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  WidgetCompositionEvent()
    : mSeqno(kLatestSeqno)
  {
  }

public:
  uint32_t mSeqno;

public:
  virtual WidgetCompositionEvent* AsCompositionEvent() MOZ_OVERRIDE
  {
    return this;
  }

  WidgetCompositionEvent(bool aIsTrusted, uint32_t aMessage,
                         nsIWidget* aWidget)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_COMPOSITION_EVENT)
    , mSeqno(kLatestSeqno)
  {
    // XXX compositionstart is cancelable in draft of DOM3 Events.
    //     However, it doesn't make sense for us, we cannot cancel composition
    //     when we send compositionstart event.
    mFlags.mCancelable = false;
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    MOZ_ASSERT(eventStructType == NS_COMPOSITION_EVENT,
               "Duplicate() must be overridden by sub class");
    // Not copying widget, it is a weak reference.
    WidgetCompositionEvent* result =
      new WidgetCompositionEvent(false, message, nullptr);
    result->AssignCompositionEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  // The composition string or the commit string.  If the instance is a
  // compositionstart event, this is initialized with selected text by
  // TextComposition automatically.
  nsString data;

  void AssignCompositionEventData(const WidgetCompositionEvent& aEvent,
                                  bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    data = aEvent.data;
  }
};

/******************************************************************************
 * mozilla::WidgetQueryContentEvent
 ******************************************************************************/

class WidgetQueryContentEvent : public WidgetGUIEvent
{
private:
  friend class dom::PBrowserParent;
  friend class dom::PBrowserChild;

  WidgetQueryContentEvent()
  {
    MOZ_CRASH("WidgetQueryContentEvent is created without proper arguments");
  }

public:
  virtual WidgetQueryContentEvent* AsQueryContentEvent() MOZ_OVERRIDE
  {
    return this;
  }

  WidgetQueryContentEvent(bool aIsTrusted, uint32_t aMessage,
                          nsIWidget* aWidget) :
    WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_QUERY_CONTENT_EVENT),
    mSucceeded(false), mWasAsync(false)
  {
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    // This event isn't an internal event of any DOM event.
    MOZ_CRASH("WidgetQueryContentEvent doesn't support Duplicate()");
    return nullptr;
  }

  void InitForQueryTextContent(uint32_t aOffset, uint32_t aLength)
  {
    NS_ASSERTION(message == NS_QUERY_TEXT_CONTENT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
    mInput.mLength = aLength;
  }

  void InitForQueryCaretRect(uint32_t aOffset)
  {
    NS_ASSERTION(message == NS_QUERY_CARET_RECT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
  }

  void InitForQueryTextRect(uint32_t aOffset, uint32_t aLength)
  {
    NS_ASSERTION(message == NS_QUERY_TEXT_RECT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
    mInput.mLength = aLength;
  }

  void InitForQueryDOMWidgetHittest(const mozilla::LayoutDeviceIntPoint& aPoint)
  {
    NS_ASSERTION(message == NS_QUERY_DOM_WIDGET_HITTEST,
                 "wrong initializer is called");
    refPoint = aPoint;
  }

  uint32_t GetSelectionStart(void) const
  {
    NS_ASSERTION(message == NS_QUERY_SELECTED_TEXT,
                 "not querying selection");
    return mReply.mOffset + (mReply.mReversed ? mReply.mString.Length() : 0);
  }

  uint32_t GetSelectionEnd(void) const
  {
    NS_ASSERTION(message == NS_QUERY_SELECTED_TEXT,
                 "not querying selection");
    return mReply.mOffset + (mReply.mReversed ? 0 : mReply.mString.Length());
  }

  bool mSucceeded;
  bool mWasAsync;
  struct
  {
    uint32_t mOffset;
    uint32_t mLength;
  } mInput;
  struct
  {
    void* mContentsRoot;
    uint32_t mOffset;
    nsString mString;
    // Finally, the coordinates is system coordinates.
    nsIntRect mRect;
    // The return widget has the caret. This is set at all query events.
    nsIWidget* mFocusedWidget;
    // true if selection is reversed (end < start)
    bool mReversed;
    // true if the selection exists
    bool mHasSelection;
    // true if DOM element under mouse belongs to widget
    bool mWidgetIsHit;
    // used by NS_QUERY_SELECTION_AS_TRANSFERABLE
    nsCOMPtr<nsITransferable> mTransferable;
  } mReply;

  enum
  {
    NOT_FOUND = UINT32_MAX
  };

  // values of mComputedScrollAction
  enum
  {
    SCROLL_ACTION_NONE,
    SCROLL_ACTION_LINE,
    SCROLL_ACTION_PAGE
  };
};

/******************************************************************************
 * mozilla::WidgetSelectionEvent
 ******************************************************************************/

class WidgetSelectionEvent : public WidgetGUIEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  WidgetSelectionEvent()
    : mSeqno(kLatestSeqno)
    , mOffset(0)
    , mLength(0)
    , mReversed(false)
    , mExpandToClusterBoundary(true)
    , mSucceeded(false)
  {
  }

public:
  uint32_t mSeqno;

public:
  virtual WidgetSelectionEvent* AsSelectionEvent() MOZ_OVERRIDE
  {
    return this;
  }

  WidgetSelectionEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_SELECTION_EVENT)
    , mSeqno(kLatestSeqno)
    , mOffset(0)
    , mLength(0)
    , mReversed(false)
    , mExpandToClusterBoundary(true)
    , mSucceeded(false)
  {
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    // This event isn't an internal event of any DOM event.
    MOZ_CRASH("WidgetSelectionEvent doesn't support Duplicate()");
    return nullptr;
  }

  // Start offset of selection
  uint32_t mOffset;
  // Length of selection
  uint32_t mLength;
  // Selection "anchor" should be in front
  bool mReversed;
  // Cluster-based or character-based
  bool mExpandToClusterBoundary;
  // true if setting selection succeeded.
  bool mSucceeded;
};

} // namespace mozilla

#endif // mozilla_TextEvents_h__

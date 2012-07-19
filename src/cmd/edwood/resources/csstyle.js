// Copyright (c) 2008  Robert Kroeger
/**
 * @fileoverview
 * Utilities for finding sepcific styles and altering them
 
 FOO
 
 */



/**
 * Given a url and a selector, stashes a reference
 * to the CSSStyleRule that corresponds.
 * @param {string} url The url to search for
 * @param {string} selector The selector to search for.
 * @constructor
 */
supragenius.CssReference = function(url, selector) {
  var sheets = document.styleSheets;
  var sheet = null;
  for (var i = 0; i < sheets.length; i++) {
    console.info(String(i) + ' href: ' + sheets[i].href);
    if (sheets[i].href.indexOf(url) != -1) {
      console.info('assigning to sheet');
      sheet = sheets[i];
      break;
    }
  }

  console.info('sheet: ' + sheet);
  if (sheet == null) {
    throw 'Could not find the requested style sheet';
  }

  this.style_ = null;

  for (i = 0; i < sheet.cssRules.length; i++ ) {
    console.info(String(i) + ' selector: ' + sheet.cssRules[i].cssText);
    if (sheet.cssRules[i].selectorText == selector) {
      this.style_ = sheet.cssRules[i];
      break;
    }
  }

  if (this.style_ == null) {
    throw 'Could not find the requested selector';
  }
};


/**
 * Returns the found CSSStyleRule
 * @return {Object}
 */
supragenius.CssReference.prototype.getCssStyle = function() {
  return this.style_;
};

// TODO(rjkroege): Alter this code to better handle the situation where
// we want to add and remove a complex style.

/**
 * Sets the background color, retaining the current color in a stack.
 * @param {string} color The color to set.
 */
supragenius.CssReference.prototype.setSelectionColor = function(color) {
  this.style_.style.backgroundColor = color;
};


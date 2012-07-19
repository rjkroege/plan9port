/*
 
 2008/8/24
 =======
 
 Need to worry about the focus behaviour...
 
 some sort of per-"div" construct that tracks the selection (hopefully I can persuade webkit to maintain selectios
 per framelet.)
 
  
 In first version with font high-lighting, we will need to manually re-fontify the buffer.

*/


/**
 * Base namespace for supragenius
 */
var supragenius = supragenius || {}; 

supragenius.logme = function(m) {
  //var loggy = document.getElementById('debuglog');
  //loggy.innerHTML += ('<li>' + m + '</li>');
	window.console.info(m);
};


supragenius.dictionaryToString = function(dict, opt_keys) {
	var result = '\n';

	var core = function(i) {
		result += '\t';
		result += String(i);
		result += ':\t\t';
		if (dict[i] != null) {
			result += String(dict[i]);
			result += '\t\t';
			result += String(dict[i].id);
		} else {
			result += 'null';
		}
		result += '\n';
	}
	
	if (opt_keys != null) {
		for(var j = 0; j < opt_keys.length; j++) {
			core(opt_keys[j]);
		}
	} else {
		for (var j in dict) {
			core(j);
		}
	}
	return	result;
};


/**
 * Dumps out a dictionary like an event..
 * TODO(rjkroege) rename this rationally.
 */
supragenius.dumpEvent = function(event) {
  var values = []
  values.push('<table>');
  for (i in event) {
    values.push('<tr>');
      values.push('<td>');
        values.push(String(i));
      values.push('</td>');
      values.push('<td>');
        values.push(event[i]);
      values.push('</td>');
    values.push('</tr>');
  }
  values.push('</table>');
  return values.join(' ');
};

/**
 * Dumps out a single line summary of a mouse event
 */
supragenius.dumpShortEvent = function(event) {
  var values = []
  var keys = ['type', 'button'];
  for (var i = 0; i < keys.length; i++) {
    values.push(keys[i]);
    values.push(': ');
    values.push(event[keys[i]]);
    values.push(', ');
  }
  return values.join('');
};


/**
 * Constructor for a new MouseStateTracker object.
 * Which is a state-machine for tracking mouse events going
 * up/down, etc.
 * @constructor
 */
supragenius.MouseStateTracker = function() {
  this.cssref_ = new supragenius.CssReference('selectionstyle.css', '::selection');
  this.tile_ = null;
};

/**
 * Sets the record of the current supra window.
 * @param {Tile?} tile The current Tile.
 * @return {Tile?} The last tile that we were in.
 */
supragenius.MouseStateTracker.prototype.setCurrentSupraTile = function(stile) {
	this.tile_ = stile;
};

/**
 * Returns the current set tile.
 * @return {Tile?}
 */
supragenius.MouseStateTracker.prototype.getCurrentSupraTile = function() {
	return this.tile_;
};

/**
  * Clears the current selection.
 */
supragenius.MouseStateTracker.deSelect = function() {
  var selection = window.getSelection();
  if (!selection.isCollapsed) {
    selection.collapseToStart();
  }
};


/** Cuts the selection from the document.  */
supragenius.MouseStateTracker.prototype.cut = function() {
	// Must also track the edits that we've actually done...
  document.execCommand('cut');
};


/**
 * Throws an exception if the given condition is fasle.
 * @param {boolean?} cond The condition to test.
 * @param {string} message A string message to fire 
 *     failure.
 */
function ASSERT(cond, message) {
  if (!cond) {
    throw message;
  }
};


/**  Pastes the current selection into the document.  */
supragenius.MouseStateTracker.prototype.paste = function() {
  supragenius.logme('Paste (replacing selection) now');
	// Recall that we must track the edits for sending to the backend.
  document.execCommand('paste');
};


/** Executes the selection or word under the cursor. */
supragenius.MouseStateTracker.prototype.executeSelection = function() {
	supragenius.logme('executeSelection');
	
	var selection = window.getSelection();
	
	if (selection.isCollapsed) {
		// grow selection
		supragenius.logme('collapsed selection');
	}
	supragenius.logme('should execute: ' + selection);
	
	// TODO(rjkroege): restore the left-mouse selection.
}

/**
 * Edge action to swallow up an event in the case that it has been
 * handled already.
 * @param {Object} event is the event to swallow.
 */
supragenius.MouseStateTracker.prototype.swallow = function(event) {
  // alert('attempting to swallow: ' + supragenius.dumpShortEvent(event));
  supragenius.logme('swallowing: ' + supragenius.dumpShortEvent(event));
  // supragenius.logme(supragenius.dumpEvent(event));
  event.preventDefault && event.preventDefault();
  event.stopImmediatePropagation && event.stopImmediatePropagation();
  event.stopPropagation && event.stopPropagation();
  event.preventBubble && event.preventBubble();
  // alert('finished swallowing: ' + supragenius.dumpShortEvent(event));
  return false;
};


/**
 * Processes a middle down event.
 */
supragenius.MouseStateTracker.prototype.execute = function() {
  supragenius.logme('should be extracting the selection and executing it');
};


/** Searches for the selection */
supragenius.MouseStateTracker.prototype.search = function() {
  supragenius.logme('should be extracting the selection and executing it');	
};


// Might need to do something else here...
supragenius.MouseStateTracker.prototype.preMiddleDown = function() {
	// this.firstMouse_ = MIDDLE;
	this.cssref_.setSelectionColor('#FF6666');
};

supragenius.MouseStateTracker.prototype.preLeftDown = function() {
	this.cssref_.setSelectionColor('#FCEC77');
}

supragenius.MouseStateTracker.prototype.preRightDown = function() {
	this.cssref_.setSelectionColor('#CCFF66');
};



var _onloadHandler = function() {
  alert('hello');
  // TODO(rjkroege) make sure that I don't turn this on
  // unstil I've built the appropriate event mappings.
  
	supragenius.mse = new supragenius.MouseStateTracker();

  // TODO(rjkroege): Set necessary event handlers here.
	// new supragenius.Tile('documentId-1', supragenius.mse);
	// new supragenius.Tile('documentId-2', supragenius.mse);

	// experiment... 
	supragenius.Tile.createAllTiles(supragenius.mse);
	
	alert('finished running _onloadHandler');
};

// TODO(rjkroege)
// insert the selection into the current clipboard entity.
var _onCutHandler = function(event) {
  supragenius.logme(supragenius.dumpEvent(event));
};


/**
 * Objective-C entry points by mouse button.
 */
function _preMd() {
	supragenius.logme('got a middle mouse down');
	supragenius.mse.preMiddleDown();
	return 'ok';
}

function _preRd() {
	supragenius.logme('got a right mouse down');
	supragenius.mse.preRightDown();
	return 'ok';
}

function _preLd() {
	supragenius.logme('got a left mouse down');
	// Set selection to be yellow...
	supragenius.mse.preLeftDown();
	return 'ok';
}

function _postMu() {
	supragenius.logme('_postMu: got a middle up: should attempt to run');
	supragenius.mse.executeSelection();
	return 'ok';
}

function _postRu() {
	supragenius.logme('_postOnRu: got a middle down');
	return 'ok';
}

function _postLdMd() {
	supragenius.logme('_postLdMd: got a middle down');
	supragenius.mse.cut();
	return 'ok';
}

function _postLdRd() {
	supragenius.logme('_postLdRd: got a right down');
	supragenius.mse.paste();
	return 'ok';
	
}

function _postMdRd() {
	supragenius.logme('_postMdRd: got a middle down');
	return 'ok';
	
}

function _postRdMd() {
	supragenius.logme('_postRdMd: got a middle down');
	return 'ok';
	
}

function _postRdLd() {
	supragenius.logme('_postRdLd: got a middle down');
	return 'ok';
	
}



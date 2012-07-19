// Copyright (c) 2008 Robert Kroeger

/**
 * @fileoverview A single on screen window is composed of multiple non-overlapping
 * SupraGenius tiles.  This class manages a single tile.  A tile implements follow focus and
 * has a selection.  
 * 
 * Tiles come in two variants: tags and tiles. Tags are single line.  Tiles are multi-line.  This
 * difference is only apparent in the CSS which gives different clipping and sizing properties
 * for this distinction.
 * 
*/


/*
 Should each til manage its own underlay. Or should there be a single underlay? 
 
 We don't need per tile names? (we need something as the hash key in the C code.)
 There needs to be a centralized painter for selection rectangles?

 It sort of depends on what makes it easier to handle resize and scroll...  
 
 
 */



/**
 * Creates a new Supragenius tile  by wrapping an existing dom node.
 * @param {Object} element The DOM element for which we are creating a Tile.
 * @param {MouseStateTracker} mse The global mouse state tracker object.
 * @constructor
 */
supragenius.Tile = function(element, mst) {
	alert('creating a Tile instance for ' + element.id);
	this.selection_ = null;
	this.mst_ = mst;
	
	// TODO(Rjkroege): the end point code would have the nodes having references
	// to the tiles.  Then, we can use the document.querySelectorAll call used below
	// to find all of the tiles.
	this.e_ = element;
	// this.e_.tile_ = this;
	this.e_.hasTile_ = true;

	// Track if we are selected?
	this.selected_ = false;
	
	// TODO(rjkroege): can cache the rectangles here for faster paint/unpaint.
	// TODO(Rjkroege): remember to toss the rectangles on resize events.
	this.bound_ = null;
	this.textRects_ = null;
	
	// Create a native tile for this tile.
	window.nativegenius.allocateTile(this.e_);
};


supragenius.Tile.prototype.bindEvents = function() {
	window.console.info('bound events for ' + this.e_.id);
	var tile = this;
	this.e_.addEventListener('mouseover', function(event) {tile.mouseEnter(event);}, false);
	this.e_.addEventListener('mouseout', function(event) {tile.mouseOut(event);}, false);
	// TODO(rjkroege): add code here to delete the cached rectangles in a resize-event handler
};


/**
  * Finds all of the DOM elements in the document that need and
 * an associated Tile object and then creates the the Tile object
 * if the element does not already have one.
 * @param {supragenius.MouseStateTracker} mst The mouse state tracker
 *		to bind to.
 */
supragenius.Tile.createAllTiles = function(mst) {
	window.console.info('hello from findTiles');
	// Construct the style underlying the body.
	document.body.style.background = '-webkit-canvas(body-underlay)';

	// TODO(rjkroege): set an initial selection (sort of contingent on mouse warping?)
	// TODO(Rjkroege): in the native code, create a default selection record.
	
	var i;
	var elementsNeedingTiles = document.querySelectorAll('div.supra-tile, div.supra-tag');
	for (i = 0; i < elementsNeedingTiles.length; i++) {
		// As a debugging measure, we have given all tiles names.
		var e = elementsNeedingTiles[i];
		// window.console.info('node: ' + e);
		window.console.info('node: ' + e.id);
		
		if (e.hasTile_ == null) {
			var tile = new supragenius.Tile(e, mst);
			tile.bindEvents();	// Preserves the existence of  the Tile object.
		}
	}
};

/*
 Notion is that we can add a general mechanism for adding annotations to tiles.  (like
 warnings from the continuous compilation server or something.
 */

/**
 * Finds if the given element is a child of this Tile's element.  An element is 
 * it's own descendent. 
 * @param {Object} e The element to test.
 * @private
 */
supragenius.Tile.prototype.isDescendent_ = function(e) {
	while (e && e != document && e != this.e_) {
		e = e.parentElement;
	}
	if (e == null || e == document) {
		return false;
	} else {
		return true;
	}
};

supragenius.Tile.prototype.getContext_ = function() {
	// TODO(rjkroege): you are using the id to create a name here. Eventually, we will
	// not have this available so this must be fixed as part of the single underlay fix
	var ctx = document.getCSSCanvasContext('2d','body-underlay', document.width, document.height);
	return ctx;
};

/**
 * Draws the background canvas for the tile appropriately for the cursor
 * being in it.  
 */
supragenius.Tile.prototype.paintFocusedBackground = function() {
	supragenius.logme('paintFocusedBackground for ' + this.e_.id );
	var ctx = this.getContext_();	

	if (this.bound_) {
		ctx.fillStyle = '#efefef';
		ctx.fillRect(this.bound_[0], this.bound_[1], this.bound_[2], this.bound_[3]);
	}

	// Test... should get a red rectangle  in the bottom right corner.
	ctx.fillStyle = '#ff0000';
	ctx.fillRect(document.width - 100, document.height - 100, 100, 100);
};

/**
 * Updates the cached selection extent rectangles from the current selection.
 * Requires the selection to be valid and corresponding to the target tile.
 */
supragenius.Tile.prototype.updateStoredRectsFromSelection = function() {
	this.bound_ = window.nativegenius.getBoundingRectangle(this.e_, true);
	supragenius.logme('set rect from native: ' + this.bound_.join(', '));
	
	this.textRects_ = window.nativegenius.getSelectionTextExtents(this.e_, true);
	
	var msg = '';
	var i;
	for (i = 0; i < this.textRects_.length; i += 4) {
		msg += [this.textRects_[i], this.textRects_[i+1], this.textRects_[i+2], this.textRects_[i+3]].join(', ');
		msg += '\n';
	}
	supragenius.logme('rects: \n' + msg);
};


/**
 * Draws the background canvas for the tile when it does not have
 * the cursor in it.  
 */
supragenius.Tile.prototype.paintUnfocusedBackground = function() {
	if (this.textRects_) {
		supragenius.logme('paintUnfocusedBackground for ' + this.e_.id);
	
		var ctx = this.getContext_();
		ctx.fillStyle = '#D4D4D4';
	
		var i;
		for (i = 0; i < this.textRects_.length; i += 4) {
			supragenius.logme('painting: ' + [this.textRects_[i], this.textRects_[i + 1], this.textRects_[i + 2], this.textRects_[i + 3]].join(', '));
			ctx.fillRect(this.textRects_[i], this.textRects_[i + 1], this.textRects_[i + 2], this.textRects_[i + 3]);
			
		}
	}
};

/*

	relatedTarget: w3c spec of the element that we came/are going to
	target: original source of the event (is bubbling)
	currentTarget: the actual element that we registered the event handler for.
 
*/ 

/**
 * Handles the mouse entering the window.
 */
supragenius.Tile.prototype.mouseEnter = function(event) {
	// TODO(learn what the event gives you...)
	// There is perhaps a reason here that I am missing...

	// I believe this to be true.
	ASSERT(event.currentTarget == this.e_);

	if (!this.isDescendent_(event.relatedTarget)) {
		supragenius.logme('mouseEnter: entering tile' + event.currentTarget.id);
		// supragenius.logme(supragenius.dictionaryToString(event, ['toElement', 'relatedTarget', 'fromElement', 'srcElement', 'target', 'currentTarget']));
		// assume that target is the node.
		var lastTile = this.mst_.getCurrentSupraTile();
		if (lastTile != this) {
			this.mst_.setCurrentSupraTile(this);
			this.restoreSelection();		
			this.paintFocusedBackground();
		}
	} 
	// else {
	//	supragenius.logme('mouseEnter: suppressing leaving / entering children nodes.');
	//}
};

// This method is superfluous -- do all logic on ENTERING a NEW window.
// Also, on re-enter, do nothing.
/**
 * Handles the mouse leaving the window.
 */
supragenius.Tile.prototype.mouseOut = function(event) {
	// I believe this to be true.
	ASSERT(event.currentTarget == this.e_);
	
	if (!this.isDescendent_(event.relatedTarget)) {
		supragenius.logme('mouseOut: leaving tile ' + event.currentTarget.id);
		//supragenius.logme(supragenius.dictionaryToString(event, ['toElement', 'relatedTarget', 'fromElement', 'srcElement',	 'target', 'currentTarget']));
		this.mst_.setCurrentSupraTile(null);
		this.updateStoredRectsFromSelection();
		this.preserveSelection();
		this.paintUnfocusedBackground();
	}
	// else {
	//	supragenius.logme('mouseOut: supressing leaving/entering to own children.');
	//}
};

/**
 * Preserve the selection on the current window.
 */
supragenius.Tile.prototype.preserveSelection = function() {
	supragenius.logme('preserveSelection');
	window.nativegenius.stashSelectionToTile(this.e_);
};

/**
 * Restore the preserved selection.
*/
supragenius.Tile.prototype.restoreSelection = function() {
	supragenius.logme('restoreSelection');
	window.nativegenius.setSelectionFromTile(this.e_);
};

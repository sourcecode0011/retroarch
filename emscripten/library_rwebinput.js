//"use strict";

var LibraryRWebInput = {
   $RI__deps: ['$Browser'],
   $RI: {
      temp: null,
      contexts: [],
      stateX: 0,
      stateY: 0,
      currentX: 0,
      currentY: 0,

      canvasEventHandler: function(event) {
         switch (event.type) {
            case 'mouseup':
            case 'mousedown':
               var value;
               var offset;
               if (event.button === 0) offset = 40;
               else if (event.button === 2) offset = 41;
               else break;
               if (event.type === 'mouseup') value = 0;
               else value = 1;
               for (var i = 0; i < RI.contexts.length; i++) {
                  {{{ makeSetValue('RI.contexts[i].state', 'offset', 'value', 'i8') }}};
               }
               break;
         }
      },

      eventHandler: function(event) {
         var i;
         switch (event.type) {
            case 'mousemove':
               var x = 0;
               var y = 0;

               var newX = event['clientX'] - Module.canvas.offsetLeft;
               var newY = event['clientY'] - Module.canvas.offsetTop;

               if (newX < 0) {
                  newX = 0;
                  x    = -Module.canvas.offsetWidth;
               } else if (newX > Module.canvas.offsetWidth) {
                  newX = Module.canvas.offsetWidth;
                  x    = Module.canvas.offsetWidth;
               } else {
                  x = newX - RI.currentX;
               }

               if (newY < 0) {
                  newY = 0;
                  y    = -Module.canvas.offsetHeight;
               } else if (newY > Module.canvas.offsetHeight) {
                  newY = Module.canvas.offsetHeight;
                  y    = Module.canvas.offsetHeight;
               } else {
                  y = newY - RI.currentY;
               }

               RI.currentX = newX;
               RI.currentY = newY;

               for (i = 0; i < RI.contexts.length; i++) {
                  {{{ makeSetValue('RI.contexts[i].state', '32', 'x', 'i32') }}};
                  {{{ makeSetValue('RI.contexts[i].state', '36', 'y', 'i32') }}};
               }

               break;

            case 'keyup':
            case 'keydown':
               var key = event.keyCode;
               var offset = key >> 3;
               var bit = 1 << (key & 7);
               if (offset >= 32) throw 'key code error! bad code: ' + key;
               for (i = 0; i < RI.contexts.length; i++) {
                  var value = {{{ makeGetValue('RI.contexts[i].state', 'offset', 'i8') }}};
                  if (event.type === 'keyup') value &= ~bit;
                  else value |= bit;
                  {{{ makeSetValue('RI.contexts[i].state', 'offset', 'value', 'i8') }}};
               }
               event.preventDefault();
               break;

            case 'blur':
            case 'visibilitychange':
               for (i = 0; i < RI.contexts.length; i++) {
                  _memset(RI.contexts[i].state, 0, 42);
               }
               break;
         }
      }
   },

   RWebInputInit: function() {
      if (RI.contexts.length === 0) {
         document.addEventListener('keyup', RI.eventHandler, false);
         document.addEventListener('keydown', RI.eventHandler, false);
         document.addEventListener('mousemove', RI.eventHandler, false);
         Module.canvas.addEventListener('mouseup', RI.canvasEventHandler, false);
         Module.canvas.addEventListener('mousedown', RI.canvasEventHandler, false);
         document.addEventListener('blur', RI.eventHandler, false);
         document.addEventListener('onvisbilitychange', RI.eventHandler, false);
      }
      if (RI.temp === null) RI.temp = _malloc(42);

      var s = _malloc(42);
      _memset(s, 0, 42);
      RI.contexts.push({
         state: s
      });
      return RI.contexts.length;
   },
   
   RWebInputPoll: function(context) {
      context -= 1;
      var state = RI.contexts[context].state;
      _memcpy(RI.temp, state, 42);
      // reset mouse movements
      {{{ makeSetValue('RI.contexts[context].state', '32', '0', 'i32') }}};
      {{{ makeSetValue('RI.contexts[context].state', '36', '0', 'i32') }}};
      return RI.temp;
   },

   RWebInputDestroy: function (context) {
      if (context === RI.contexts.length) {
         RI.contexts.pop();
         if (RI.contexts.length === 0) {
            document.removeEventListener('keyup', RI.eventHandler, false);
            document.removeEventListener('keydown', RI.eventHandler, false);
            document.removeEventListener('mousemove', RI.eventHandler, false);
            Module.canvas.removeEventListener('mouseup', RI.canvasEventHandler, false);
            Module.canvas.removeEventListener('mousedown', RI.canvasEventHandler, false);
            document.removeEventListener('blur', RI.eventHandler, false);
            document.removeEventListener('onvisbilitychange', RI.eventHandler, false);
         }
      }
   }
};

autoAddDeps(LibraryRWebInput, '$RI');
mergeInto(LibraryManager.library, LibraryRWebInput);

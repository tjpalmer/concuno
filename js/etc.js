// Exports.


exports.$class = $class;
exports.clone = clone;


// Functions.


function $class(current, $super, builder) {
  if ($super) {
    Prototype.prototype = $super.prototype;
    current.prototype = new Prototype;
    current['super'] = $super;
  }
  if (builder) {
    builder.call(current.prototype);
  }
  // Prototype constructor.
  function Prototype() {
    this.constructor = current;
  }
}


// TODO Move clone out to some utility area.
// Cloning technique based on examples and discussions here:
// http://stackoverflow.com/questions/122102/
//   what-is-the-most-efficient-way-to-clone-a-javascript-object
function clone(obj) {
  // This could be more sophisticated about more types, but eh.
  if (obj == null || typeof(obj) != 'object') {
    return obj;
  }
  var copied = new obj.constructor;
  for (var key in obj) {
    var val = obj[key];
    copied[key] = clone(val);
  }
  return copied;
}

exports.$class = $class;

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

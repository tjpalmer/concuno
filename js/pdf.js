// Imports.


var $class = require('./etc').$class;


// Exports.


exports.Gaussian = Gaussian;


// Classes.


function Gaussian() {

  this.mean = null;
  this.cov = null;

} $class(Gaussian, null, function() {

  this.eval = function(x) {
    // TODO Implement!
    return NaN;
  }

});

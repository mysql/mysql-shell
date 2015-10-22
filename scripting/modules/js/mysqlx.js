/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

exports.mysqlx = {}

// Connection functions
exports.mysqlx.getSession = function(connection_data, password)
{
  var session;

  if (typeof(password) == 'undefined')
    session = _F.mysqlx.XSession(connection_data);
  else
    session = _F.mysqlx.XSession(connection_data, password);
  
  return session;
}

exports.mysqlx.getNodeSession = function(connection_data, password)
{
  var session;
  
  if (typeof(password) == 'undefined')
    session = _F.mysqlx.NodeSession(connection_data);
  else
    session = _F.mysqlx.NodeSession(connection_data, password);
  
  return session;
}


exports.mysqlx.expr = function(expression)
{
	if (typeof(expression) == 'undefined')
		expr_obj = _F.mysqlx.Expression();
	else
		expr_obj = _F.mysqlx.Expression(expression);
	
  return expr_obj
}

// Data Type Constants
exports.mysqlx.TinyInt = _F.mysqlx.Constant('DataTypes', 'TinyInt');
exports.mysqlx.SmallInt = _F.mysqlx.Constant('DataTypes', 'SmallInt');
exports.mysqlx.MediumInt = _F.mysqlx.Constant('DataTypes', 'MediumInt');
exports.mysqlx.Int = _F.mysqlx.Constant('DataTypes', 'Int');
exports.mysqlx.Integer = _F.mysqlx.Constant('DataTypes', 'Integer');
exports.mysqlx.BigInt = _F.mysqlx.Constant('DataTypes', 'BigInt');
exports.mysqlx.Real = _F.mysqlx.Constant('DataTypes', 'Real');
exports.mysqlx.Float = _F.mysqlx.Constant('DataTypes', 'Float');
exports.mysqlx.Double = _F.mysqlx.Constant('DataTypes', 'Double');
exports.mysqlx.Date = _F.mysqlx.Constant('DataTypes', 'Date');
exports.mysqlx.Time = _F.mysqlx.Constant('DataTypes', 'Time');
exports.mysqlx.Timestamp = _F.mysqlx.Constant('DataTypes', 'Timestamp');
exports.mysqlx.DateTime = _F.mysqlx.Constant('DataTypes', 'DateTime');
exports.mysqlx.Year = _F.mysqlx.Constant('DataTypes', 'Year');
exports.mysqlx.Bit = _F.mysqlx.Constant('DataTypes', 'Bit');

// Data Type Functions
exports.mysqlx.Text = function(length){
	var t;
	if (typeof(length) == 'undefined')
		t = _F.mysqlx.Constant('DataTypes', 'Text');
	else
		t = _F.mysqlx.Constant('DataTypes', 'Text', length);
	return t;
}


exports.mysqlx.Blob = function(length){
	var b;
	if (typeof(length) == 'undefined')
		b = _F.mysqlx.Constant('DataTypes', 'Blob');
	else
		b = _F.mysqlx.Constant('DataTypes', 'Blob', length);
	return b;
}



exports.mysqlx.Decimal = function(precision, scale){
	var decimal;
	if (typeof(precision) == 'undefined' && typeof(scale) == 'undefined')
		decimal = _F.mysqlx.Constant('DataTypes', 'Decimal');
	else if (typeof(scale) == 'undefined')
		decimal = _F.mysqlx.Constant('DataTypes', 'Decimal', precision);
	else
		decimal = _F.mysqlx.Constant('DataTypes', 'Decimal', precision, scale);
	
	return decimal;
}

exports.mysqlx.Numeric = function(precision, scale){
	var numeric;
	if (typeof(precision) == 'undefined' && typeof(scale) == 'undefined')
		numeric = _F.mysqlx.Constant('DataTypes', 'Numeric');
	else if (typeof(scale) == 'undefined')
		numeric = _F.mysqlx.Constant('DataTypes', 'Numeric', precision);
	else
		numeric = _F.mysqlx.Constant('DataTypes', 'Numeric', precision, scale);
	
	return numeric;
}


// Index Type Constants
exports.mysqlx.IndexUnique = _F.mysqlx.Constant('IndexTypes', 'IndexUnique');

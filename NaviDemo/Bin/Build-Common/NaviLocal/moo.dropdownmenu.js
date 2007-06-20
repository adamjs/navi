Element.extend(
{
	hide: function() 
	{
		this.setStyle('display', 'none');
	},
	
	show: function() 
	{
		
		this.setStyle('display', '');
	}
});

/*	
Script: moo.dropdownmenu.js
	Fx.DropdownMenu.
		
Dependencies:
	<Moo.js>, <Function.js>, <Array.js>, <String.js>, <Element.js>

Author:
	André Fiedler, <http://visualdrugs.net>

License:
	MIT-style license.

Class: Fx.PopMenu
	The DropdownMenu function creates a group of elements that have the behaviour of an dropdown menu.
	
Arguments:
	element - a list-element the effect will be applied to.
	
Example:
	(start code)
	<ul id="dropdownMenu">
		<li>
			<a href="#">Menu 1</a>
			<ul>
				<li><a href="#">SubMenu 1</a></li>
				<li><a href="#">SubMenu 2</a></li>
				<li><a href="#">SubMenu 3</a></li>	
			</ul>
		</li>
		<li><a href="#">Menu 2</a></li>
		<li><a href="#">Menu 3</a></li>	
	</ul>
	
	<script type="text/javascript">
	
		Window.onDomReady(function() {new DropdownMenu($('dropdownMenu'))});
		
	</script>
	(end)
*/

var DropdownMenu = new Class({	
	initialize: function(element)
	{
		$A($(element).childNodes).each(function(el)
		{
			if(el.nodeName.toLowerCase() == 'li')
			{
				$A($(el).childNodes).each(function(el2)
				{
					if(el2.nodeName.toLowerCase() == 'ul')
					{
						$(el2).hide();
						
						el.addEvent('mouseover', function()
						{
							el2.show();
							return false;
						});

						el.addEvent('mouseout', function()
						{
							el2.hide();
						});
						new DropdownMenu(el2);
					}
				});
			}
		});
		return this;
	}
});
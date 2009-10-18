Require(Basic.Flammable)
Require(Basic.Item)

register_type{Basic.Item, Basic.Flammable;
	name = "Book",
   namespace = Default,
	 attribs = {
		 burnable = 2,
		 remove_on_burn = 1,  -- Non-nil causes it to be removed after burning
		 --burn_vertex = GRIP_RIGHT,
		 --burn_Offset = Vector{0.5, 0.5, 0},
		 
		 new = function(self, bookName, bookContents )
			 self.name = bookName
			 self.bookContents = bookContents
		 end,
		 
		apply_Impact = function (self, action, result)
			-- FIXME: Make it check damage type first
			self:ignite()
		end,
		 
	 },
	alerts = {
		spawn = function (self)
				-- fall to the ground if we're not being held by someone
			if not self.attached_to then
				self:animate(%ACTIONJB)
				self.keep_action = 1
			end
			
		end,
		
		used  = function( self )
			message("You read " .. self.name .. ": " .. self.bookContents )
		end,
	},
	profile = {
		model = "std.items.books:tris.md2",
		bump_size = 1,
		bump_height = 0,
		is_item = 1,
		weight = Range{2,3},
		weapon_attack = SLICE,
		skins = {
			normal = Skin{
				name = "book",
				texture = "std.items.books:tris0.bmp",
				icon = "std.items.books:icon0.bmp"
			}
		}
	}
}

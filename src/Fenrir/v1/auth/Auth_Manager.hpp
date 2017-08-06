/*
 * Copyright (c) 2014-2015, Luca Fulchir<luca@fulchir.it>, All rights reserved.
 *
 * This file is part of libFenrir.
 *
 * libFenrir is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * libFenrir is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and a copy of the GNU Lesser General Public License
 * along with libFenrir.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "../libFenrir.hpp"
#include "Username.hpp"
#include "../Utils/hash.hpp"

namespace Fenrir {
namespace Auth {

class FENRIR_LOCAL Lattice_Node
{
public:
	std::vector<uint8_t> name;
	std::vector<std::weak_ptr<Lattice_Node>> parents;
	std::vector<std::shared_ptr<Lattice_Node>> children;
	uint8_t id;

	Lattice_Node (uint8_t _id, const std::vector<uint8_t> &_name)
		:name(_name), id(_id) {}

	std::vector<uint8_t> serialize();
};

class FENRIR_LOCAL Lattice
{
public:
	Lattice (const std::vector<uint8_t> &raw);

	operator bool() const;		// check validity of parsed lattice
	bool exists (uint8_t id);
	bool includes (uint8_t parent, uint8_t child);
	std::vector<uint8_t> serialize();
private:
	std::shared_ptr<Lattice_Node> top;

	std::shared_ptr<Lattice_Node> find (std::shared_ptr<Lattice_Node> from,
										uint8_t id, std::vector<uint8_t> &out);
};

class FENRIR_LOCAL Manager
{
	// Manage lattices, authentication tokens, usernames.

	// TODO: static length -> array
	using Lattice_ID = std::vector<uint8_t>; // 128 bit = 16 bytes;
public:
	bool add_lattice (const Lattice_ID &id, const std::vector<uint8_t> &raw);
	bool del_lattice (const Lattice_ID &id);
	std::shared_ptr<Lattice> get_lattice (const Lattice_ID &id);

	bool add_token (const Username &auth_user, const Username &service_user,
											const std::vector<uint8_t> &token,
											const uint16_t service_id,
											const Lattice_ID &lattice,
											const uint8_t lattice_node);
	bool del_token (const Username &auth_user, const Username &service_user,
													const uint16_t service_id,
													const uint8_t lattice_node);
	const std::vector<uint8_t> get_token (const Username &auth_user,
													const Username service_user,
													const uint16_t service_id,
													const uint8_t lattice_node);
	// TODO: get the whole list
private:
	class FENRIR_LOCAL service_info {
	public:
		Username service_user;
		Lattice_ID lattice;
		std::vector<uint8_t> token;
		uint16_t service_id;
		uint8_t lattice_node;
		bool operator== (const service_info &other) {
			// token and lattice_id are superfluous for comparison
			return service_id != other.service_id &&
					lattice_node != other.lattice_node &&
					service_user != other.service_user;
		}
		bool operator!= (const service_info &other) {
			return !operator==(other);
		}
	};
	std::mutex lattice_mtx, auth_mtx;
	std::unordered_map<Lattice_ID, std::shared_ptr<Lattice>> lattices;
	// username -> service and token information
	// Each (auth)username can have multiple tokens to connect to
	// multiple services (with multiple usernames)
	std::unordered_map<Username, std::vector<service_info>> auths;
};

}
}

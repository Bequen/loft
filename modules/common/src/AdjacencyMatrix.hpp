#pragma once

#include <algorithm>
#include <cstdint>

#include <random>
#include <vector>
#include <map>

#include "Assert.h"

/**
 * Represents a graph as an adjacency matrix.
 */
struct AdjacencyMatrix {
private:
	std::vector<std::vector<bool>> m_matrix;
	std::map<std::string, uint32_t> m_node_idx;
	std::vector<std::string> m_node_names;

	void find_dft(uint32_t node, uint32_t target, uint32_t maxDepth);

public:
	explicit AdjacencyMatrix(std::vector<std::string> node_names) :
		m_matrix(node_names.size(), std::vector<bool>(node_names.size())),
		m_node_names(node_names) 
	{
		for(uint32_t x = 0; x < node_names.size(); x++) {
			m_node_idx.insert({node_names[x], x});
	}
}

	bool has_loop() {
		return false;
	}
	
	const std::vector<std::vector<bool>>& rows () const {
		return m_matrix;
	}

	const std::vector<bool>& row(const std::string& name) const {
		ASSERT(m_node_idx.contains(name));
		return m_matrix[m_node_idx.at(name)];
	}


	[[nodiscard]] [[deprecated("Use string variant")]] bool get(uint32_t from, uint32_t to) const {
		ASSERT(from < m_matrix.size() && to < m_matrix.size());

		return m_matrix[from][to];
	}

	[[nodiscard]] bool get(const std::string& from, const std::string& to) const {
		ASSERT(m_node_idx.contains(from) && m_node_idx.contains(to));
		return m_matrix[m_node_idx.at(from)][m_node_idx.at(to)];
	}

	inline AdjacencyMatrix& set(uint32_t from, uint32_t to) {
		ASSERT(from < m_matrix.size() && to < m_matrix.size());

		m_matrix[from][to] = true;
		return *this;
	}

	inline AdjacencyMatrix& unset(uint32_t from, uint32_t to) {
		ASSERT(from < m_matrix.size() && to < m_matrix.size());

		m_matrix[from][to] = false;
		return *this;
	}

	inline AdjacencyMatrix& set(const std::string& from, const std::string& to) {
		ASSERT(m_node_idx.contains(from) && m_node_idx.contains(to));

		m_matrix[m_node_idx.at(from)][m_node_idx.at(to)] = true;
		return *this;
	}

	inline AdjacencyMatrix& unset(const std::string& from, const std::string& to) {
		ASSERT(m_node_idx.contains(from) && m_node_idx.contains(to));

		m_matrix[m_node_idx.at(from)][m_node_idx.at(to)] = false;
		return *this;
	}


	/**
	 * Counts and returns number of dependencies of item at 'to' index
	 * @param to Index of the dependant
	 * @return number representing count of dependencies
	 */
	[[nodiscard]] uint32_t num_dependencies(uint32_t to) const {
		ASSERT(to < m_matrix.size());

		uint32_t numDependencies = 0;
		for(uint32_t x = 0; x < m_matrix.size(); x++) {
			if(get(x, to)) {
				numDependencies++;
			}
		}

		return numDependencies;
	}

	[[nodiscard]] uint32_t num_dependencies_of(const std::string& node) const {
		return num_dependencies(m_node_idx.at(node));
	}

	/**
	 * Gets all the dependencies of 'to' item as vector
	 * @param to Index of the dependant
	 * @return vector of indices of dependencies
	 */
	[[nodiscard]] std::vector<uint32_t> get_dependencies(uint32_t to) const {
		ASSERT(to < m_matrix.size());

		uint32_t numDependencies = num_dependencies(to);
		std::vector<uint32_t> dependencies(numDependencies);
		uint32_t i = 0;

		for(uint32_t x = 0; x < m_matrix.size() && i < numDependencies; x++) {
			if(get(x, to)) {
				dependencies[i++] = x;
			}
		}

		return dependencies;
	}

	[[nodiscard]] std::vector<std::string> get_dependencies(const std::string& to) const {
		uint32_t numDependencies = num_dependencies(m_node_idx.at(to));
		std::vector<std::string> dependencies(numDependencies);
		uint32_t i = 0;

		for(uint32_t x = 0; x < m_matrix.size() && i < numDependencies; x++) {
			if(get(m_node_names[x], to)) {
				dependencies[i++] = m_node_names[x];
			}
		}

		return dependencies;
	}

	[[nodiscard]] std::vector<std::string> get_successors(const std::string& from) const {
		std::vector<std::string> successors;
		uint32_t i = 0;

		for(auto& node : m_node_names) {
			if(get(from, node)) {
				successors.push_back(node);
			}
		}

		return successors;
	}

	/**
	 * Does a transitive reduction on the matrix.
	 */
	void transitive_reduction();

	void print();
};

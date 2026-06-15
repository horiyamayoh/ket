#pragma once

/**
 * @file ket_container.h
 * @brief 標準コンテナの小さいlookup、生成、削除補助API。
 *
 * @details `find`、`end`、erase-remove、default値取得など標準コンテナ利用時の
 * 反復的な手順を短い名前へ集約する。ヘッダオンリーmoduleのため、drop-in時は
 * ヘッダ単体で持ち出す。`std::optional`を使わず、C++11でnull、default、factory
 * 生成の方針を明示する。
 *
 * @par プロジェクトへの適用方法
 * `ket_container.h` を対象プロジェクトへコピー。ヘッダオンリーmodule。
 *
 * @par C++バージョン要件
 * 最小要件：C++11。
 * 本ライブラリの適用を推奨する C++ バージョン：C++11以降。
 * 推奨理由：map/vector の反復的な lookup と erase-remove 手順を標準ライブラリのみで薄く包める。
 * 本ライブラリの適用を推奨しない C++ バージョン：なし。
 * 非推奨理由：なし。
 *
 * @par 他のライブラリへの依存
 * 標準ライブラリのみ。
 * 他のket moduleへの依存なし。
 *
 * @par namespace
 * 公開API：ket::container
 * 内部実装：ket::container::detail
 */

#include <algorithm> // IWYU pragma: keep
#include <cstddef>
#include <iterator>

namespace ket
{
	namespace container
	{
		// -----------------------------------------------------------------------------
		// Public API declarations
		// -----------------------------------------------------------------------------

		/**
		 * @brief range内の値存在確認。
		 * @tparam Container `begin`と`end`で走査できるコンテナ型。
		 * @tparam Value 検索値の型。
		 * @param[in] container 検索対象のコンテナ。
		 * @param[in] value 検索値。
		 * @retval true `value`と等値比較される要素あり。
		 * @retval false `value`と等値比較される要素なし。
		 * @pre `container`は有効なrange。要素と`value`の等値比較が有効。
		 * @post 引数と外部状態の変更なし。iterator取得や等値比較の例外は呼び出し元へ伝播。
		 * @code
		 * const auto found = ket::container::Contains(values, 42);
		 * // found == true if values contains 42
		 * @endcode
		 */
		template <typename Container, typename Value>
		bool Contains(const Container& container, const Value& value);

		/**
		 * @brief map内のkey存在確認。
		 * @tparam Map `find`と`end`を持つmap型。
		 * @tparam Key 検索keyの型。
		 * @param[in] map 検索対象のmap。
		 * @param[in] key 検索key。
		 * @retval true `key`に対応する要素あり。
		 * @retval false `key`に対応する要素なし。
		 * @pre `map.find(key)`が有効。
		 * @post 引数と外部状態の変更なし。lookup時の例外は呼び出し元へ伝播。
		 */
		template <typename Map, typename Key>
		bool ContainsKey(const Map& map, const Key& key);

		/**
		 * @brief map内の値へのmutable pointer取得。
		 * @tparam Map `mapped_type`、`find`、`end`を持つmap型。
		 * @tparam Key 検索keyの型。
		 * @param[in,out] map 検索対象のmap。返却pointer経由で値変更可能。
		 * @param[in] key 検索key。
		 * @retval value `key`に対応するmap内要素へのpointer。
		 * @retval nullptr `key`に対応する要素なし。
		 * @pre `map.find(key)`が有効。
		 * @post map構造の変更なし。返却pointerは標準コンテナ規則に従ってinvalidation。
		 * @note copyを避け、不在をnullptrで表すためpointerを返す。
		 */
		template <typename Map, typename Key>
		typename Map::mapped_type* AtOrNull(Map& map, const Key& key);

		/**
		 * @brief map内の値へのconst pointer取得。
		 * @tparam Map `mapped_type`、`find`、`end`を持つmap型。
		 * @tparam Key 検索keyの型。
		 * @param[in] map 検索対象のmap。
		 * @param[in] key 検索key。
		 * @retval value `key`に対応するmap内要素へのconst pointer。
		 * @retval nullptr `key`に対応する要素なし。
		 * @pre `map.find(key)`が有効。
		 * @post map構造の変更なし。返却pointerは標準コンテナ規則に従ってinvalidation。
		 * @note copyを避け、不在をnullptrで表すためpointerを返す。
		 */
		template <typename Map, typename Key>
		const typename Map::mapped_type* AtOrNull(const Map& map, const Key& key);

		/**
		 * @brief map内の値またはdefault値取得。
		 * @tparam Map `mapped_type`、`find`、`end`を持つmap型。
		 * @tparam Key 検索keyの型。
		 * @param[in] map 検索対象のmap。
		 * @param[in] key 検索key。
		 * @param[in] default_value `key`が無い場合に返す値。
		 * @retval value `key`があればmap内要素のcopy、無ければ`default_value`。
		 * @pre `map.find(key)`が有効。`Map::mapped_type`はcopyまたはmoveで戻り値化可能。
		 * @post 引数と外部状態の変更なし。lookupやcopy/moveの例外は呼び出し元へ伝播。
		 */
		template <typename Map, typename Key>
		typename Map::mapped_type
		AtOr(const Map& map, const Key& key, typename Map::mapped_type default_value);

		/**
		 * @brief map内の値取得またはfactory生成。
		 * @tparam Map `mapped_type`、`find`、`end`、`emplace`を持つmap型。
		 * @tparam Key 検索keyの型。
		 * @tparam Factory 引数なしで`Map::mapped_type`へ変換可能な値を返すfactory型。
		 * @param[in,out] map 検索と必要時の挿入対象。
		 * @param[in] key 検索key。
		 * @param[in] factory `key`が無い場合だけ呼ぶ生成処理。
		 * @retval value 既存または新規挿入されたmap内要素への参照。
		 * @pre `map.find(key)`と`map.emplace(key, factory())`が有効。
		 * @post 既存keyではmap構造を変更せず、factory呼び出しなし。keyが無い場合は生成値を挿入。
		 * 生成、lookup、挿入の例外は呼び出し元へ伝播。
		 */
		template <typename Map, typename Key, typename Factory>
		typename Map::mapped_type& AtOrCreate(Map& map, const Key& key, Factory factory);

		/**
		 * @brief 条件に一致するsequence要素の削除。
		 * @tparam Sequence `begin`、`end`、`size`、range `erase`を持つsequence型。
		 * @tparam Predicate 要素を受け取りboolへ変換可能な結果を返す述語型。
		 * @param[in,out] sequence 削除対象のsequence。
		 * @param[in] predicate 削除対象ならtrueを返す述語。
		 * @retval value 削除した要素数。
		 * @pre `std::remove_if`と`sequence.erase(first, last)`が有効。
		 * @post `predicate`がtrueを返した要素を削除し、相対順序は標準erase-removeの性質に従う。
		 * 述語や要素移動の例外は呼び出し元へ伝播。
		 */
		template <typename Sequence, typename Predicate>
		std::size_t EraseIf(Sequence& sequence, Predicate predicate);

		/**
		 * @brief vector-like値列のsortと重複削除。
		 * @tparam Vector random access iterator、`erase`、`value_type`比較を持つvector-like型。
		 * @param[in,out] values sortと重複削除の対象。
		 * @retval void 戻り値なし。
		 * @pre `std::sort`、`std::unique`、`values.erase(first, last)`が有効。
		 * @post `values`は昇順に並び、等値の重複要素は1つだけ残る。
		 * 比較、移動、eraseの例外は呼び出し元へ伝播。
		 */
		template <typename Vector>
		void SortUnique(Vector& values);

		// -----------------------------------------------------------------------------
		// Internal implementation details
		// -----------------------------------------------------------------------------

		namespace detail
		{
			/**
			 * @brief map lookup iterator取得。
			 * @tparam Map 検索対象map型。
			 * @tparam Key 検索keyの型。
			 * @param[in,out] map 検索対象のmap。
			 * @param[in] key 検索key。
			 * @retval value `key`に対応するiterator、または`map.end()`。
			 * @pre `map.find(key)`が有効。
			 * @post map構造の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename Map, typename Key>
			typename Map::iterator Find(Map& map, const Key& key)
			{
				return map.find(key);
			}

			/**
			 * @brief const map lookup iterator取得。
			 * @tparam Map 検索対象map型。
			 * @tparam Key 検索keyの型。
			 * @param[in] map 検索対象のmap。
			 * @param[in] key 検索key。
			 * @retval value `key`に対応するconst_iterator、または`map.end()`。
			 * @pre `map.find(key)`が有効。
			 * @post map構造の変更なし。
			 * @note detail配下の関数は公開APIではない。
			 */
			template <typename Map, typename Key>
			typename Map::const_iterator Find(const Map& map, const Key& key)
			{
				return map.find(key);
			}

		} // namespace detail

		// -----------------------------------------------------------------------------
		// Public API definitions
		// -----------------------------------------------------------------------------

		template <typename Container, typename Value>
		bool Contains(const Container& container, const Value& value)
		{
			using std::begin;
			using std::end;

			const auto first = begin(container);
			const auto last = end(container);
			const auto found = std::find(first, last, value);

			return found != last;
		}

		template <typename Map, typename Key>
		bool ContainsKey(const Map& map, const Key& key)
		{
			const auto found = detail::Find(map, key);
			const auto last = map.end();

			return found != last;
		}

		template <typename Map, typename Key>
		typename Map::mapped_type* AtOrNull(Map& map, const Key& key)
		{
			const auto found = detail::Find(map, key);
			const auto last = map.end();
			const auto key_exists = found != last;
			if (!key_exists)
			{
				return nullptr;
			}

			return &found->second;
		}

		template <typename Map, typename Key>
		const typename Map::mapped_type* AtOrNull(const Map& map, const Key& key)
		{
			const auto found = detail::Find(map, key);
			const auto last = map.end();
			const auto key_exists = found != last;
			if (!key_exists)
			{
				return nullptr;
			}

			return &found->second;
		}

		template <typename Map, typename Key>
		typename Map::mapped_type
		AtOr(const Map& map, const Key& key, typename Map::mapped_type default_value)
		{
			const auto found = detail::Find(map, key);
			const auto last = map.end();
			const auto key_exists = found != last;
			if (!key_exists)
			{
				return default_value;
			}

			return found->second;
		}

		template <typename Map, typename Key, typename Factory>
		typename Map::mapped_type& AtOrCreate(Map& map, const Key& key, Factory factory)
		{
			const auto found = detail::Find(map, key);
			const auto last = map.end();
			const auto key_exists = found != last;
			if (key_exists)
			{
				return found->second;
			}

			const auto insertion = map.emplace(key, factory());

			return insertion.first->second;
		}

		template <typename Sequence, typename Predicate>
		std::size_t EraseIf(Sequence& sequence, Predicate predicate)
		{
			const auto old_size = sequence.size();
			const auto removed_begin = std::remove_if(sequence.begin(), sequence.end(), predicate);
			sequence.erase(removed_begin, sequence.end());
			const auto new_size = sequence.size();

			return old_size - new_size;
		}

		template <typename Vector>
		void SortUnique(Vector& values)
		{
			std::sort(values.begin(), values.end());
			const auto duplicate_begin = std::unique(values.begin(), values.end());
			values.erase(duplicate_begin, values.end());
		}

	} // namespace container

} // namespace ket

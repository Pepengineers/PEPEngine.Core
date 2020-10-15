#pragma once

#include "pch.h"

namespace PEPEngine
{
	namespace Allocator
	{
		template <class T>
		class DoublyLinkedList
		{
		public:
			struct Node
			{
				T data;
				Node* previous;
				Node* next;
			};

			Node* head;
		public:
			DoublyLinkedList();

			void insert(Node* previousNode, Node* newNode);
			void remove(Node* deleteNode);
		private:
			DoublyLinkedList(DoublyLinkedList& doublyLinkedList);
		};
	}
}

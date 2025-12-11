//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef _YamlWriter_h
#define _YamlWriter_h

#include <iostream>
#include <stack>

class YamlWriter
{
  struct Block
  {
    int Indent;
    bool IsList;
    bool AtListItemStart;

    Block(int indent)
      : Indent(indent)
      , IsList(false)
      , AtListItemStart(false)
    {
    }
  };

  std::ostream& OutputStream;
  std::stack<Block> BlockStack;
  bool AtBlockStart;

  Block& CurrentBlock() { return this->BlockStack.top(); }

  const Block& CurrentBlock() const { return this->BlockStack.top(); }

  void WriteIndent()
  {
    int indent = this->CurrentBlock().Indent;
    bool listStart = this->CurrentBlock().AtListItemStart;

    if (listStart)
    {
      --indent;
    }

    for (int i = 0; i < indent; ++i)
    {
      this->OutputStream << "  ";
    }

    if (listStart)
    {
      this->OutputStream << "- ";
      this->CurrentBlock().AtListItemStart = false;
    }
  }

public:
  explicit YamlWriter(std::ostream& outputStream = std::cout)
    : OutputStream(outputStream)
    , AtBlockStart(true)
  {
    this->BlockStack.emplace(0);
  }

  ~YamlWriter()
  {
    if (this->BlockStack.size() != 1)
    {
      throw std::runtime_error("YamlWriter destroyed before last block complete.");
    }
  }

  /// Starts a block underneath a dictionary item. The key for the block is
  /// given, and the contents of the block, which can be a list or dictionary
  /// or list of dictionaries and can contain sub-blocks, is created by calling
  /// further methods of this class.
  ///
  /// A block started with \c StartBlock _must_ be ended with \c EndBlock.
  ///
  void StartBlock(const std::string& key)
  {
    this->WriteIndent();
    this->OutputStream << key << ":" << std::endl;

    int indent = this->CurrentBlock().Indent;
    this->BlockStack.push(Block(indent + 1));
    this->AtBlockStart = true;
  }

  /// Finishes a block.
  ///
  void EndBlock()
  {
    this->BlockStack.pop();
    this->AtBlockStart = false;

    if (this->BlockStack.empty())
    {
      throw std::runtime_error("Ended a block that was never started.");
    }
  }

  /// Start an item in a list. Can be a dictionary item.
  ///
  void StartListItem()
  {
    Block& block = this->CurrentBlock();
    if (block.IsList)
    {
      if (!block.AtListItemStart)
      {
        block.AtListItemStart = true;
      }
      else
      {
        // Ignore empty list items
      }
    }
    else if (this->AtBlockStart)
    {
      // Starting a list.
      block.IsList = true;
      block.AtListItemStart = true;
      ++block.Indent;
    }
    else
    {
      throw std::runtime_error("Tried to start a list in the middle of a yaml block.");
    }
  }

  /// Add a list item that is just a single value.
  ///
  void AddListValue(const std::string& value)
  {
    this->StartListItem();
    this->WriteIndent();
    this->OutputStream << value << std::endl;
    this->AtBlockStart = false;
  }

  /// Add a key/value pair for a dictionary entry.
  ///
  template <typename T>
  void AddDictionaryEntry(const std::string& key, const T& value)
  {
    this->WriteIndent();
    this->OutputStream << key << ": " << value << std::endl;
    this->AtBlockStart = false;
  }
};

#endif //_YamlWriter_h

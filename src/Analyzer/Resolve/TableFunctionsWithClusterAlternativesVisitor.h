#pragma once

#include <Analyzer/InDepthQueryTreeVisitor.h>
#include <Analyzer/TableFunctionNode.h>
#include <TableFunctions/TableFunctionFactory.h>
#include <TableFunctions/TableFunctionFile.h>

namespace DB
{

class TableFunctionsWithClusterAlternativesVisitor : public InDepthQueryTreeVisitor<TableFunctionsWithClusterAlternativesVisitor, /*const_visitor=*/true>
{
public:
    void visitImpl(const QueryTreeNodePtr & node)
    {
        auto * table_function_node = node->as<TableFunctionNode>();
        if (!table_function_node)
            return;

        auto table_function_name = table_function_node->getTableFunctionName();
        if (table_function_name == TableFunctionFile::name)
            return;

        auto potential_cluster_table_function_name = table_function_name + "Cluster";

        if (TableFunctionFactory::instance().isTableFunctionName(potential_cluster_table_function_name))
            table_functions.push_back(node);
    }

    bool needChildVisit(const QueryTreeNodePtr &, const QueryTreeNodePtr &) { return true; }
    std::vector<QueryTreeNodePtr> getTableFunctions() { return table_functions; }

private:
    std::vector<QueryTreeNodePtr> table_functions;
};

}

# Filename: lldb_formatter_dataframe.py
# Copyright 2024 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

import lldb

class DataFrameSyntheticProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.update()
        print(f"Applying formatter to variable: {self.valobj.GetName()}")

    def update(self):
        # Retrieve elements of the DataFrame for display
        self.data_type = self.valobj.GetType().GetTemplateArgumentType(0).GetName()
        self.layout = self.valobj.GetType().GetTemplateArgumentType(3).GetName()
        self.backing_res = self.valobj.GetChildMemberWithName("backingRes")
        self.capacity = self.valobj.GetChildMemberWithName("capacity")
        self.flds = self.valobj.GetChildMemberWithName("flds")
        self.recs = self.valobj.GetChildMemberWithName("recs")
        self.recsData = self.valobj.GetChildMemberWithName("recsData")

    def get_summary(self):
        """Generate a structured summary for the DataFrame object."""

        # Data type and layout
        data_type_summary = f"DataType:\t\t{self.data_type}"
        layout_summary = f"Layout:\t\t\t{self.layout}"

        # Memory resource info (assuming unique pointer address as identifier)
        mem_res_summary = f"Memory Resource:\t{self.backing_res.GetValue()}"

        # Field and record counts
        field_count = self.flds.GetNumChildren()
        record_count = self.recs.GetNumChildren()
        field_count_summary = f"Field Count:\t\t{field_count}"
        record_count_summary = f"Record Count:\t\t{record_count}"

        # Truncated view of recsData (10x10 max)
        recs_data_summary = "Records:\n"
        row_count = min(10, self.recsData.GetChildMemberWithName("extent_0").GetValueAsUnsigned())
        col_count = min(10, self.recsData.GetChildMemberWithName("extent_1").GetValueAsUnsigned())

        for row in range(row_count):
            row_data = []
            for col in range(col_count):
                element = self.recsData.GetChildAtIndex(row * col_count + col)
                row_data.append(element.GetValue())
            recs_data_summary += "\t\t" + "\t".join(row_data) + "\n"

        # Concatenate all parts of the summary
        return (
            f"{data_type_summary}\n"
            f"{layout_summary}\n"
            f"{mem_res_summary}\n\n"
            f"{field_count_summary}\n"
            f"{record_count_summary}\n\n"
            f"{recs_data_summary}"
        )

    def num_children(self):
        return self.valobj.GetNumChildren()

    def get_child_at_index(self, index):
        return self.valobj.GetChildAtIndex(index)

    def has_children(self):
        return True

def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand('type synthetic add -x "^lgz::DataFrame<.*>$" --python-class DataFrameFormatter.DataFrameSyntheticProvider -w cplusplus')
    debugger.HandleCommand('type summary add -x "^lgz::DataFrame<.*>$" -F DataFrameFormatter.DataFrameSyntheticProvider.get_summary -w cplusplus')
    debugger.HandleCommand('type category enable cplusplus')
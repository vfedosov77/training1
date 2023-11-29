from typing import List, Tuple
from bisect import bisect_left
class Grid:
    class Node:
        def __init__(self, range: Tuple[float, float], size: int, coord: List[int]):
            self.coord = list.copy(coord)
            self.children = [None for _ in range(size)]
            self.value = None
            range_step = (range[1] - range[0]) / size
            self.children_range_starts = [range[0] + (range_step * i) for i in range(size)]

        def __iter__(self):
            if self.children[0] is None:
                yield self

            for child in self.children:
                for leaf in child:
                    yield leaf

    def __init__(self, ranges: List[Tuple[float, float]], cells_per_range: 5):
        self.ranges_count = len(ranges)
        self.cells_per_range = cells_per_range

        coord = []
        self.node = Node(ranges[0], cells_per_range, coord)
        self._add_children(self.node, cells_per_range, 0, coord)

    def _add_children(self, parent, ranges, cells_per_range, parent_range_id, coord: List):
        if parent_range_id == len(ranges):
            return
        coord.append(0)

        for j in range(cells_per_range):
            coord[-1] = parent.children_range_starts[i]
            parent.children[j] = Node(ranges[parent_range_id + 1], cells_per_range)
            self._add_children(parent.children[j], cells_per_range, parent_range_id + 1, coord)

        coord.pop()

    def add_point(self, coords):
        node = self.node

        for i in range(self.ranges_count):
            node_id = bisect_left(node.children_range_starts, coords[i])

            if node_id > self.cells_per_range:
                node_id = self.cells_per_range - 1

            node = node.children[node_id]

        if node.value is None:
            node.value = coords

    def __iter__(self):
        for leaf in self.node:
            yield leaf

# TODO: bandwidth functions

import matplotlib.pyplot as plt
from itertools import repeat
import numpy as np
from collections import defaultdict

from raw_json_reader import RawJsonReader


class RawJsonPlotter:
    def __init__(self, img_dir, path):
        self.img_dir = img_dir
        self.reader = RawJsonReader(path)

        self.latencies = self.reader.get_latencies()
        self.timestamps = self.reader.get_timestamps()
        self.types = self.reader.get_types()
        self.thread_ids = self.reader.get_thread_ids()

    def latency_of_same_thread(self):
        colors = list()
        latencies = self.latencies

        for i, t in enumerate(self.types):
            if t == "read":
                colors.extend(repeat("r", 2))
                plt.plot(self.timestamps[i * 2: i * 2 + 2], [latencies[i], latencies[i]], color="r")
            elif t == "write":
                colors.extend(repeat("b", 2))
                plt.plot(self.timestamps[i * 2: i * 2 + 2], [latencies[i], latencies[i]], color="b")
            else:  # pause
                colors.extend(repeat("w", 2))

                # resize latency to size of x
                latencies.insert(i, 0)

                # add dotted line and its timestamp on left side
                plt.axvline(x=self.timestamps[i * 2], color="g", linestyle="--")
                plt.annotate(self.timestamps[i * 2],
                             xy=(self.timestamps[i * 2], latencies[i]),
                             xytext=(self.timestamps[i * 2], max(latencies) / 4),
                             ha="left",
                             color="g")

                # add dotted line and its timestamp on right side
                plt.axvline(x=self.timestamps[i * 2 + 1], color="g", linestyle="--")
                plt.annotate(self.timestamps[i * 2 + 1],
                             xy=(self.timestamps[i * 2 + 1], latencies[i]),
                             xytext=(self.timestamps[i * 2 + 1], max(latencies) / 4),
                             ha="right",
                             color="g")

        # add legend
        plt.plot(0, 0, color="r", label="read")
        plt.plot(0, 0, color="b", label="write")
        plt.plot(0, 0, color="g", linestyle="--", label="pause")
        plt.legend(loc="upper left", bbox_to_anchor=(1, 1))

        # duplicate values in latency for following scatter plot
        latencies = [latencies[l // 2] for l in range(len(latencies) * 2)]

        plt.scatter(self.timestamps, latencies, c=colors, s=5)
        plt.subplots_adjust(left=.15)
        plt.ylim(bottom=0)
        plt.xlabel("Timestamps")
        plt.ylabel("Latency (ns)")

        plt.savefig(self.img_dir + self.reader.get_benchmark_name() + "-latency-singlethreaded-raw.png", bbox_inches="tight")
        plt.close()

    def latency_of_several_threads(self):
        if self.reader.contains_pauses():  # TODO: viz for several threads and pauses
            pass

        else:  # TODO: second viz with lines for threads on y-axis and time on x-axis
            average_latency_per_thread = defaultdict(list)
            for i, l in enumerate(self.latencies):
                average_latency_per_thread[self.thread_ids[i]].append(l)

            # ensure that average latency is always the first value of each key
            for lpt in average_latency_per_thread.values():
                if len(lpt) > 1:
                    lpt[0] = sum(lpt) / len(lpt)

            # actual bar plot
            plt.bar(sorted(average_latency_per_thread.keys()),
                    [values[0] for values in sorted(average_latency_per_thread.values())])

            # only plot x-ticks for every thread ID
            plt.xticks(np.arange(
                min(sorted(average_latency_per_thread.keys())), max(sorted(average_latency_per_thread.keys())) + 1))

            plt.subplots_adjust(left=.15)
            plt.ylim(bottom=0)
            plt.xlabel("Thread IDs")
            plt.ylabel("Average Latency (ns)")

            plt.savefig(self.img_dir + self.reader.get_benchmark_name() + "-latency-multithreaded-raw.png", bbox_inches="tight")
            plt.close()

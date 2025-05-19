import h5py
import matplotlib.pyplot as plt
import matplotlib.widgets as widgets
import numpy as np
from tkinter import filedialog
import argparse
import sys
import os

#pyinstaller.exe --onefile --windowed .\h5_plot3a.py
def select_file():
    return filedialog.askopenfilename(title="Select HDF5 file", filetypes=[("HDF5 files", "*.h5 *.hdf5")])

def main(file=None):
    if file is None:
        file = select_file()
        if not file:
            print("No file selected. Exiting.")
            return

    # Open the HDF5 file
    with h5py.File(file, 'r') as f:
        # Get the dataset
        dset = f['my_dataset']

        # Get the shape of the dataset
        nchan, nsamp = dset.shape

        # Set the chunk size
        chunk_size = 2560

        # Create a figure with a 4x4 grid of subplots
        fig, axs = plt.subplots(4, 4, figsize=(12, 12), sharex=True, sharey=True)

        # Initialize the plots
        lines = []
        for i, ax in enumerate(axs.flat):
            line, = ax.plot(dset[i, :chunk_size])
            lines.append(line)
            ax.set_title(f'Channel {i+1}')
            ax.set_xlabel('Sample Index')
            ax.set_ylabel('Amplitude')

        # Create a slider to select the point in the file to plot
        ax_slider = plt.axes([0.2, 0.05, 0.4, 0.03])
        slider = widgets.Slider(ax_slider, 'Sample Index', 0, nsamp - chunk_size, valinit=0, valfmt='%1.15g')

        # Define the update function for the slider
        def update(val):
            k = int(slider.val)
            for i, line in enumerate(lines):
                line.set_ydata(dset[i, k:k+chunk_size])
            fig.canvas.draw_idle()

        # Register the update function with the slider
        slider.on_changed(update)

        # Create buttons to move forward and backward one chunk
        ax_prev = plt.axes([0.05, 0.02, 0.05, 0.075])
        ax_next = plt.axes([0.85, 0.02, 0.05, 0.075])
        btn_prev = widgets.Button(ax_prev, 'Prev')
        btn_next = widgets.Button(ax_next, 'Next')

        # Define the functions to move forward and backward one chunk
        def prev_chunk(event):
            slider.set_val(max(0, slider.val - chunk_size))

        def next_chunk(event):
            slider.set_val(min(nsamp - chunk_size, slider.val + chunk_size))

        # Register the functions with the buttons
        btn_prev.on_clicked(prev_chunk)
        btn_next.on_clicked(next_chunk)

        # Set the title of the figure to the file name
        fig.suptitle(f'Plot of {file}')

        # Show the plot
        plt.tight_layout()
        plt.subplots_adjust(bottom=0.2)  # Make room for the slider and buttons
        plt.show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='4x4 Grid of Time Domain Plots')
    parser.add_argument('file', nargs='?', help='Path to the HDF5 file')
    args = parser.parse_args()
    
    program_name = os.path.basename(sys.argv[0])
    program_name_without_extension, _ = os.path.splitext(program_name)
    info_file_name = f"{program_name_without_extension}.info"

    with open(info_file_name, 'w') as f:
        f.write(f"fname=4x4 Grid of Time Domain Plots\n")
        f.write("version=0.1\n")
        f.write("info=Provides block increment and slider for indexing\n")
        f.write(f"author=jpietzuch@pfinc.com\n")
        f.write("date=08/19/2024\n")
        
    # Check if a file argument was supplied
    if args.file is None:
        print('Error: Please provide a path to an HDF5 file.', file=sys.stderr)
        sys.exit(1)

    main(args.file)
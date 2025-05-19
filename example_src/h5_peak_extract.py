import os,sys
import h5py
import numpy as np
import argparse
import time

def process_dataset(dset, output_file, chunk_size=1024, sr=1.0):
    nchan, nsamp = dset.shape
    start_time = time.time()
    last_print_time = start_time
    with open(output_file, 'w') as f:
        for chan_idx in range(nchan):
            peak_amps = np.zeros(chunk_size // 2 + 1)
            peak_freqs = np.zeros(chunk_size // 2 + 1)
            print(f"Channel {chan_idx+1}:")
            sys.stdout.flush()
            f.write(f"Channel {chan_idx+1}:\n")
            for i in range(0, nsamp, chunk_size):
                chunk = dset[chan_idx, i:i+chunk_size]
                if len(chunk) < chunk_size:
                    chunk = np.pad(chunk, (0, chunk_size - len(chunk)), mode='constant')

                fft_out = np.fft.rfft(chunk)
                amplitudes = np.abs(fft_out[:chunk_size // 2 + 1]) / chunk_size
                frequencies = np.fft.rfftfreq(chunk_size, d=1.0/sr)[:chunk_size // 2 + 1]

                peak_amps = np.maximum(peak_amps, amplitudes)
                peak_freqs = np.where(peak_amps == amplitudes, frequencies, peak_freqs)

            idx = np.where(peak_amps > 0.05)[0]
            peak_amps = peak_amps[idx]
            peak_freqs = peak_freqs[idx]

            # Sort by amplitude in descending order
            sort_idx = np.argsort(-peak_amps)
            peak_amps = peak_amps[sort_idx]
            peak_freqs = peak_freqs[sort_idx]

            # Sort by frequency in ascending order
            sort_idx = np.argsort(peak_freqs)
            peak_amps = peak_amps[sort_idx]
            peak_freqs = peak_freqs[sort_idx]

            # Show only the top 16
            for i in range(min(16, len(peak_amps))):
                print(f"  Peak Amplitude: {peak_amps[i]:.2f}, Frequency: {peak_freqs[i]:.2f} Hz")
                sys.stdout.flush()
                f.write(f"  Peak Amplitude: {peak_amps[i]:.2f}, Frequency: {peak_freqs[i]:.2f} Hz\n")

            current_time = time.time()
            if current_time - last_print_time >= 0.5:
                progress = (chan_idx + 1) / nchan * 100
                print(f"Processing channel {chan_idx+1} of {nchan} ({progress:.2f}% complete)")
                sys.stdout.flush()
                f.write(f"Processing channel {chan_idx+1} of {nchan} ({progress:.2f}% complete)\n")
                last_print_time = current_time

def main():
    parser = argparse.ArgumentParser(description='Perform FFT peak hold on a hdf5 file and produce a text file output')
    parser.add_argument('file', nargs='?', help='Path to the HDF5 file')
    args = parser.parse_args()
    
    program_name = os.path.basename(sys.argv[0])
    program_name_without_extension, _ = os.path.splitext(program_name)
    info_file_name = f"{program_name_without_extension}.info"

    with open(info_file_name, 'w') as f:
        f.write(f"fname=Perform FFT peak hold on a hdf5 file and produce a text file output\n")
        f.write(f"version=0.1\n")
        f.write(f"info=Perform FFT peak hold on a hdf5 file and produce a text file output\n")
        f.write(f"author=jpietzuch@pfinc.com\n")
        f.write(f"date=08/19/2024\n")
        
    # Check if a file argument was supplied
    if args.file is None:
        print('Error: Please provide a path to an HDF5 file.', file=sys.stderr)
        sys.exit(1)

    input_file_name, _ = os.path.splitext(args.file)
    output_file_name = f"{input_file_name}-{program_name_without_extension}.txt"

    with h5py.File(args.file, 'r') as f:
        dset = f['my_dataset']
        sample_rate = dset.attrs['sample_rate']
        print(sample_rate)
        attr = dset.attrs['sample_rate_measured']
        print(attr)
        attr = dset.attrs['sample_rate_changed_count']
        print(attr)
        process_dataset(dset, output_file_name, 32768, sample_rate)
        channel_names = dset.attrs['channel_names']
        measurement_units = dset.attrs['measurement_units']

        # Print out

        # Print out the channel names
        for name in channel_names:
            print(name.decode('utf-8'))
        for name in measurement_units:
            print(name.decode('utf-8'))
            
    print(f"exported_filepath={output_file_name}\n")
    sys.stdout.flush()

if __name__ == '__main__':
    main()